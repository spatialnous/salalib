# SPDX-FileCopyrightText: 2026 Petros Koutsolampros
#
# SPDX-License-Identifier: GPL-3.0-or-later

YEAR=$(date +%Y)
IDENT="$(git config user.name) <$(git config user.email)>"
WHO=$(git check-mailmap "$IDENT" 2>/dev/null | sed 's/ <.*//')
[ -n "$WHO" ] || { echo "cannot determine committer name"; exit 1; }
WHO_RE=$(printf '%s' "$WHO" | sed 's/[][\.*^$(){}?+|/]/\\&/g')

FAIL=0
for f in $(git diff --cached --name-only --diff-filter=d); do
    [ -f "$f" ] || continue
    grep -q 'SPDX-FileCopyrightText' "$f" 2>/dev/null || continue     # REUSE.toml-covered
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