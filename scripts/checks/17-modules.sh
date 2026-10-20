#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The module graph is declared in cmake/module-deps.txt, and the code keeps to
# it (#194). The check fails when
#   - the table is malformed, names an unknown module, leaves a module out or
#     has a cycle;
#   - an #include in src/<dir>/include/ or src/<dir>/src/ reaches a module or
#     a Qt module the table does not give that module (a public edge, for an
#     installed header). A private header from src/common/ counts as written
#     in the file that includes it, so it cannot carry an edge in unseen;
#   - an installed header includes a private one, or the install export file
#     names a path in the source tree;
#   - a QtOpenAi target's links and the table differ, either way, visibility
#     included.
#
# The links are CMake's own, not a parse of the LIBS lists: a configure-only
# tree in "$QTOPENAI_BUILD_DIR-modules", deleted before every run so nothing
# from an earlier one can change the verdict, with a snippet that dumps each
# target's LINK_LIBRARIES and INTERFACE_LINK_LIBRARIES. Shared, so a PRIVATE
# link stays out of the interface instead of becoming $<LINK_ONLY:...>.
#
# Without cmake, on Windows, or with a Qt that lacks a module's dependency, the
# rules that can run do, and the check then skips -- a FAIL under --strict.
# Written for bash 3.2: the tables and the graph are awk's.

set -euo pipefail

export LC_ALL=C
root=$QTOPENAI_ROOT
cd "$root"
table=cmake/module-deps.txt

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

failed=0
report() {
    [[ -s $1 ]] || return 0
    sort -t: -k1,1 -k2,2n -k3 "$1" | uniq
    failed=1
}

# Directory -> module, from the qtopenai_add_module call that defines it.
for cml in src/*/CMakeLists.txt; do
    dir=${cml#src/}
    dir=${dir%/CMakeLists.txt}
    names=$(sed -nE 's/^[[:space:]]*qtopenai_add_module\(([A-Za-z0-9_]+).*/\1/p' "$cml")
    [[ -n $names ]] || continue
    if [[ $names == *$'\n'* ]]; then
        echo "$cml: more than one qtopenai_add_module call" >&2
        exit 1
    fi
    echo "$dir $names" >>"$tmp/modules"
done
if [[ ! -s $tmp/modules ]]; then
    echo "no qtopenai_add_module call found under src/" >&2
    exit 1
fi
if [[ ! -f $table || ! -r $table ]]; then
    echo "$table: missing or unreadable" >&2
    exit 1
fi
modules=$(wc -l <"$tmp/modules" | tr -d ' ')

# --- The table: syntax, names, completeness, cycles --------------------------
: >"$tmp/edges"
awk -v modfile="$tmp/modules" -v edgefile="$tmp/edges" -v table="$table" '
    function sortwords(s,    w, n, i, j, x, out) {
        n = split(s, w, " ")
        for (i = 2; i <= n; i++) {
            x = w[i]
            for (j = i - 1; j >= 1 && w[j] > x; j--) w[j + 1] = w[j]
            w[j + 1] = x
        }
        out = ""
        for (i = 1; i <= n; i++) out = out (i > 1 ? " " : "") w[i]
        return out
    }
    BEGIN {
        while ((getline l < modfile) > 0) {
            split(l, a, " ")
            known[a[2]] = 1
            names = names " " a[2]
        }
        nm = split(sortwords(names), mod, " ")
    }
    {
        line = $0
        sub(/#.*/, "", line)
        nf = split(line, f, " ")
        if (nf == 0) next
        where = table ":" NR ": "
        if (nf != 3) { print where "expected <Module> <Dependency> <public|private>"; next }
        m = f[1]; d = f[2]; v = f[3]; ok = 1
        if (m in known) hasline[m] = 1
        else { print where "unknown module " m; ok = 0 }
        if (!(d in known) && d !~ /^Qt6::[A-Za-z0-9_]+$/) { print where "unknown dependency " d; ok = 0 }
        if (v != "public" && v != "private") { print where "visibility " v " is neither public nor private"; ok = 0 }
        if (m == d) { print where m " depends on itself"; ok = 0 }
        if ((m, d) in seen) { print where "duplicate " m " " d " (also line " seen[m, d] ")"; ok = 0 }
        else seen[m, d] = NR
        if (ok) {
            print m, d, v, NR > edgefile
            if (d in known) adj[m, d] = reach[m, d] = 1
        }
    }
    END {
        for (i = 1; i <= nm; i++)
            if (!(mod[i] in hasline)) print table ": module " mod[i] " has no line"
        # The modules on a cycle, by transitive closure; then the shortest
        # cycle through the first of them, in name order, to name it.
        for (k = 1; k <= nm; k++)
            for (i = 1; i <= nm; i++)
                if ((mod[i], mod[k]) in reach)
                    for (j = 1; j <= nm; j++)
                        if ((mod[k], mod[j]) in reach) reach[mod[i], mod[j]] = 1
        for (i = 1; i <= nm; i++) {
            s = mod[i]
            if (!((s, s) in reach) || (s in done)) continue
            on = ""
            for (j = 1; j <= nm; j++)
                if (((s, mod[j]) in reach) && ((mod[j], s) in reach)) { done[mod[j]] = 1; on = on " " mod[j] }
            split("", prev); split("", queue)
            head = 1; tail = 1; queue[1] = s; last = ""
            while (head <= tail && last == "") {
                u = queue[head++]
                for (j = 1; j <= nm && last == ""; j++) {
                    w = mod[j]
                    if (!((u, w) in adj)) continue
                    if (w == s) last = u
                    else if (!(w in prev)) { prev[w] = u; queue[++tail] = w }
                }
            }
            path = s
            for (u = last; u != s; u = prev[u]) path = u " -> " path
            print table ": modules on a cycle:" on " (" s " -> " path ")"
        }
    }
' "$table" >"$tmp/table.err"
report "$tmp/table.err"
edges=$(wc -l <"$tmp/edges" | tr -d ' ')

# --- The #include lines ------------------------------------------------------
# git's files, as format and duplication see them: tracked or untracked, not
# ignored.
: >"$tmp/files"
while IFS= read -r -d '' file; do
    [[ -f $file ]] && printf '%s\n' "$file" >>"$tmp/files"
done < <(git ls-files -z --cached --others --exclude-standard -- 'src/*.h' 'src/*.cpp' | sort -z)
if [[ ! -s $tmp/files ]]; then
    echo "no .h/.cpp files found under src/ -- git ls-files failed?" >&2
    exit 1
fi
files=$(wc -l <"$tmp/files" | tr -d ' ')

awk -v modfile="$tmp/modules" -v edgefile="$tmp/edges" '
    function sortwords(s,    w, n, i, j, x, out) {
        n = split(s, w, " ")
        for (i = 2; i <= n; i++) {
            x = w[i]
            for (j = i - 1; j >= 1 && w[j] > x; j--) w[j + 1] = w[j]
            w[j + 1] = x
        }
        out = ""
        for (i = 1; i <= n; i++) out = out (i > 1 ? " " : "") w[i]
        return out
    }
    function iscommon(name,    x, r) {
        if (!(name in common)) {
            r = (getline x < ("src/common/" name))
            close("src/common/" name)
            common[name] = (r >= 0)
        }
        return common[name]
    }
    function need(d, shown, where,    v, list) {
        v = ((M, d) in vis) ? vis[M, d] : ""
        if (v == "public" || (v == "private" && !pub)) return
        list = pub ? pubdeps[M] : alldeps[M]
        print where M " includes " shown "; " M " may depend on: " (list == "" ? "(nothing)" : list)
    }
    function classify(l, where, ln,    s, delim, t, rest, x) {
        if (l !~ /^[ \t]*#[ \t]*include/) return
        if (!match(l, /^[ \t]*#[ \t]*include[ \t]*[<"][^>"]*[>"]/)) { print where "unparsed include"; return }
        s = substr(l, RSTART, RLENGTH)
        sub(/^[ \t]*#[ \t]*include[ \t]*/, "", s)
        delim = substr(s, 1, 1)
        t = substr(s, 2, length(s) - 2)
        if (index(t, "../") || substr(t, 1, 1) == "/") { print where "relative include"; return }
        if (substr(t, 1, 9) == "QtOpenAi/") {
            rest = substr(t, 10)
            x = index(rest, "/") ? substr(rest, 1, index(rest, "/") - 1) : rest
            if (!(x in known)) print where "unknown module " x
            else if (x != M) need(x, rest, where)
            return
        }
        if (delim == "<" && t ~ /^Qt[A-Za-z0-9_]+\//) {
            need("Qt6::" substr(t, 3, index(t, "/") - 3), "<" t ">", where)
            return
        }
        if (delim == "\"" && !index(t, "/") && ((t in expanded) || iscommon(t))) {
            if (pub) print where "installed header includes the private header " t
            if (!(t in expanded)) {
                expanded[t] = 1
                scan("src/common/" t, ln)
            }
            return
        }
        if (pub && t ~ /_p\.h$/) { print where "installed header includes the private header " t; return }
        if (delim == "<" && !index(t, "/") && (t ~ /^Q/ || t ~ /^q.*\.h$/)) print where "use <QtModule/Header>"
    }
    # Lines of a common header are reported at the line that included it.
    function scan(file, at,    l, n, r) {
        n = 0
        while ((r = (getline l < file)) > 0) {
            n++
            classify(l, path ":" (at ? at : n) ": ", at ? at : n)
        }
        if (r < 0) print path ": cannot read " file
        close(file)
    }
    BEGIN {
        while ((getline l < modfile) > 0) {
            split(l, a, " ")
            moddir[a[1]] = a[2]
            known[a[2]] = 1
        }
        while ((getline l < edgefile) > 0) {
            split(l, a, " ")
            vis[a[1], a[2]] = a[3]
            alldeps[a[1]] = alldeps[a[1]] " " a[2]
            if (a[3] == "public") pubdeps[a[1]] = pubdeps[a[1]] " " a[2]
        }
        for (m in alldeps) alldeps[m] = sortwords(alldeps[m])
        for (m in pubdeps) pubdeps[m] = sortwords(pubdeps[m])
    }
    {
        path = $0
        n = split(path, p, "/")
        if (p[2] == "common") next
        if (n < 4 || !(p[2] in moddir) || (p[3] != "include" && p[3] != "src")) {
            print path ": not under the include/ or src/ of a module"
            next
        }
        M = moddir[p[2]]
        # A module with no line is already a table error.
        if (!(M in alldeps)) next
        pub = (p[3] == "include")
        split("", expanded)
        scan(path, 0)
    }
' "$tmp/files" >"$tmp/include.err"
report "$tmp/include.err"

summary() {
    echo "modules: $modules modules, $edges edges, $files files scanned"
}

# --- The link lists and the install export, from a configure -----------------
command -v cmake >/dev/null || {
    echo "cmake not found -- link and export rules skipped"
    summary
    [[ $failed -eq 0 ]] && exit 77
    exit 1
}
case $(uname -s) in
    MINGW* | MSYS* | CYGWIN*)
        # C:/ and /c/ spell the same source path, which the export rule
        # searches for, and no CI job runs this check on Windows.
        echo "not run on Windows -- link and export rules skipped"
        summary
        [[ $failed -eq 0 ]] && exit 77
        exit 1
        ;;
esac

tree=$QTOPENAI_BUILD_DIR-modules
dump=$tree/module-links.txt
rm -rf "$tree"

# Qt queues DEFER calls of its own; deferring once more makes the dump the
# last thing the top directory runs.
cat >"$tmp/dump.cmake" <<EOF
function(_qtopenai_modules_dump)
    set(out "")
    set(dirs "\${CMAKE_CURRENT_SOURCE_DIR}")
    list(LENGTH dirs left)
    while(left GREATER 0)
        list(POP_FRONT dirs dir)
        get_property(targets DIRECTORY "\${dir}" PROPERTY BUILDSYSTEM_TARGETS)
        foreach(target IN LISTS targets)
            if(NOT target MATCHES "^QtOpenAi")
                continue()
            endif()
            string(APPEND out "T \${target}\n")
            foreach(property IN ITEMS LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
                get_target_property(linked \${target} \${property})
                if(NOT linked)
                    continue()
                endif()
                string(SUBSTRING "\${property}" 0 1 tag)
                foreach(entry IN LISTS linked)
                    string(APPEND out "\${tag} \${entry}\n")
                endforeach()
            endforeach()
        endforeach()
        get_property(subdirs DIRECTORY "\${dir}" PROPERTY SUBDIRECTORIES)
        list(APPEND dirs \${subdirs})
        list(LENGTH dirs left)
    endwhile()
    file(WRITE [==[$dump]==] "\${out}")
endfunction()
function(_qtopenai_modules_defer)
    cmake_language(DEFER CALL _qtopenai_modules_dump)
endfunction()
cmake_language(DEFER CALL _qtopenai_modules_defer)
EOF

# The build's Qt while it exists; CMake searches again for a Qt6_DIR without a
# config file in it.
qt=()
cache=$QTOPENAI_BUILD_DIR/CMakeCache.txt
if [[ -f $cache ]]; then
    qt6_dir=$(sed -n 's/^Qt6_DIR:[A-Z]*=//p' "$cache" | tr -d '\r')
    [[ -n $qt6_dir ]] && qt=("-DQt6_DIR=$qt6_dir")
fi
unset CMAKE_BUILD_TYPE
generator=(-G "Unix Makefiles")
command -v ninja >/dev/null && generator=(-G Ninja)

if ! cmake -S "$root" -B "$tree" "${generator[@]}" ${qt[@]+"${qt[@]}"} \
    -DCMAKE_PROJECT_QtOpenAi_INCLUDE="$tmp/dump.cmake" \
    -DQTOPENAI_BUILD_SHARED=ON -DQTOPENAI_INSTALL=ON \
    -DQTOPENAI_BUILD_TESTS=OFF -DQTOPENAI_BUILD_EXAMPLES=OFF >"$tmp/configure.log" 2>&1; then
    tail -n 30 "$tmp/configure.log"
    echo "configure of $tree failed -- refused"
    summary
    exit 1
fi
if ! grep -q '^T ' "$dump" 2>/dev/null; then
    tail -n 30 "$tmp/configure.log"
    echo "configure of $tree wrote no link dump -- refused"
    summary
    exit 1
fi

# The install export, in whichever layout this CMake writes it (3.21:
# Export/lib/cmake/QtOpenAi/, from 3.24: Export/<hash>/).
: >"$tmp/export.err"
find "$tree/CMakeFiles/Export" -type f -name QtOpenAiTargets.cmake 2>/dev/null | sort >"$tmp/exports"
count=$(wc -l <"$tmp/exports" | tr -d ' ')
if [[ $count -ne 1 ]]; then
    echo "expected one QtOpenAiTargets.cmake under $tree/CMakeFiles/Export, found $count -- refused"
    summary
    exit 1
fi
export_file=$(cat "$tmp/exports")
for file in "$export_file" "${export_file%.cmake}"-*.cmake; do
    [[ -f $file ]] || continue
    { grep -n -F -e "$root" -e "$(pwd -P)" -e /src/common "$file" || true; } |
        while IFS= read -r hit; do
            printf '%s:%s (a source path in the install export)\n' "${file#"$tree"/}" "$hit"
        done >>"$tmp/export.err"
done
report "$tmp/export.err"

: >"$tmp/missing"
awk -v modfile="$tmp/modules" -v edgefile="$tmp/edges" -v table="$table" -v missing="$tmp/missing" '
    function sortwords(s,    w, n, i, j, x, out) {
        n = split(s, w, " ")
        for (i = 2; i <= n; i++) {
            x = w[i]
            for (j = i - 1; j >= 1 && w[j] > x; j--) w[j + 1] = w[j]
            w[j + 1] = x
        }
        out = ""
        for (i = 1; i <= n; i++) out = out (i > 1 ? " " : "") w[i]
        return out
    }
    BEGIN {
        while ((getline l < modfile) > 0) {
            split(l, a, " ")
            names = names " " a[2]
        }
        nm = split(sortwords(names), mod, " ")
        while ((getline l < edgefile) > 0) {
            split(l, a, " ")
            ne++
            em[ne] = a[1]; ed[ne] = a[2]; ev[ne] = a[3]; el[ne] = a[4]
            vis[a[1], a[2]] = a[3]
            allows[a[1]] = allows[a[1]] " " a[2] ":" a[3]
        }
    }
    $1 == "T" { t = $2; target[++nt] = t; next }
    { e = substr($0, 3) }
    $1 == "L" && !((t, e) in inL) { inL[t, e] = 1; L[t, ++nl[t]] = e }
    $1 == "I" && !((t, e) in inI) { inI[t, e] = 1; I[t, ++ni[t]] = e }
    END {
        for (k = 1; k <= nt; k++) {
            t = target[k]
            m = substr(t, 9)
            if (!(m in allows)) { print "target " t " has no module in the table"; continue }
            present[m] = 1
            for (i = 1; i <= nl[t]; i++) {
                e = L[t, i]
                if (index(e, "$<")) { print "target " t " links " e ": a generator expression"; continue }
                if (e ~ /^QtOpenAi::[A-Za-z0-9_]+$/) d = substr(e, 11)
                else if (e ~ /^Qt6::[A-Za-z0-9_]+$/) d = e
                else { print "target " t " links " e ": neither QtOpenAi::* nor Qt6::*"; continue }
                v = ((t, e) in inI) ? "public" : "private"
                if ((m, d) in vis && vis[m, d] == v) linked[m, d] = 1
                else print "target " t " links " e " " toupper(v) "; table allows: " sortwords(allows[m])
            }
            for (i = 1; i <= ni[t]; i++)
                if (!((t, I[t, i]) in inL)) print "target " t " links " I[t, i] " INTERFACE only"
        }
        for (i = 1; i <= ne; i++)
            if ((em[i] in present) && !((em[i], ed[i]) in linked))
                print table ":" el[i] ": " em[i] " " ed[i] " " ev[i] " is declared but QtOpenAi" em[i] " does not link it " ev[i]
        for (i = 1; i <= nm; i++)
            if ((mod[i] in allows) && !(mod[i] in present)) print mod[i] > missing
    }
' "$dump" >"$tmp/links.err"
report "$tmp/links.err"

if [[ -s $tmp/missing ]]; then
    echo "not configured with this Qt (a Qt module it needs is missing): $(tr '\n' ' ' <"$tmp/missing" | sed 's/ $//')"
    summary
    [[ $failed -eq 0 ]] && exit 77
    exit 1
fi
summary
exit $failed
