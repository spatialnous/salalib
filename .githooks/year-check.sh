# SPDX-FileCopyrightText: 2026 Petros Koutsolampros
#
# SPDX-License-Identifier: GPL-3.0-or-later

YEAR=$(date +%Y)
IDENT="$(git config user.name) <$(git config user.email)>"
WHO=$(git check-mailmap "$IDENT" 2>/dev/null | sed 's/ <.*//')
[ -n "$WHO" ] || { echo "cannot determine committer name"; exit 1; }
WHO_RE=$(printf '%s' "$WHO" | sed 's/[][\.*^$(){}?+|/]/\\&/g')

# Changed lines that do not, on their own, warrant a copyright year bump. A file
# whose staged diff touches only lines matching this ERE is skipped. Mechanical
# include hygiene is the motivating case: adding a transitive #include is not an
# act of authorship. Override before sourcing to add patterns, or set it empty to
# check every change:
#   YEAR_CHECK_IGNORE_PATTERNS='' .githooks/pre-commit
# n.b. "=" not ":=", so an explicitly empty value disables the feature rather
# than falling back to this default.
: "${YEAR_CHECK_IGNORE_PATTERNS=^[[:space:]]*#[[:space:]]*include|^[[:space:]]*$}"

FAIL=0
for f in $(git diff --cached --name-only --diff-filter=d "${EXTRA_YEAR_CHECK_EXCLUDES[@]}"); do
    [ -f "$f" ] || continue
    grep -q 'SPDX-FileCopyrightText:' "$f" 2>/dev/null || continue     # REUSE.toml-covered
    if [ -n "$YEAR_CHECK_IGNORE_PATTERNS" ]; then
        # -U0 so only changed lines appear; drop the +++/--- headers, strip the
        # leading +/- marker. If nothing is left that fails the ignore patterns,
        # the file changed in no way worth attributing.
        CHANGED=$(git diff --cached -U0 -- "$f" |
                  grep -E '^[+-]' | grep -Ev '^(\+\+\+|---) ' | cut -c2-)
        printf '%s\n' "$CHANGED" | grep -qEv "$YEAR_CHECK_IGNORE_PATTERNS" || continue
    fi
    LINE=$(grep -nE "SPDX-FileCopyrightText:.*${WHO_RE}" "$f" | head -1)
    if [ -z "$LINE" ]; then
        [ $FAIL -eq 0 ] && echo "Copyright headers need updating for $YEAR:"; FAIL=1
        echo "  $f"
        echo "      no copyright line for $WHO - add one"
        continue
    fi
    printf '%s' "$LINE" | grep -qE "SPDX-FileCopyrightText:.*${YEAR}.*${WHO_RE}" && continue
    [ $FAIL -eq 0 ] && echo "Copyright headers need updating for $YEAR:"; FAIL=1
    N=${LINE%%:*}; TEXT=${LINE#*:}
    YEARS=$(printf '%s' "$TEXT" | sed -E 's/.*SPDX-FileCopyrightText: ([0-9]{4}(-[0-9]{4})?).*/\1/')
    NEW=$(printf '%s' "$YEARS" | sed -E "s/^([0-9]{4})(-[0-9]{4})?$/\1-$YEAR/")
    echo "  $f:$N"
    echo "      $(printf '%s' "$TEXT" | sed 's/^[[:space:]]*//')"
    echo "      -> change '$YEARS' to '$NEW'"
done
[ $FAIL -eq 0 ] || exit 1