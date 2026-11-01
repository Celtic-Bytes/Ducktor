#!/usr/bin/env bash
# Uploads a file as an asset of a Forgejo release, replacing any asset that
# already has the same display name, so a re-run of the release workflow never
# fails on a duplicate nor leaves duplicates behind.
#
# Usage: upload-asset.sh <release-id> <file> [display-name]
# Requires the environment: GITHUB_API_URL, GITHUB_REPOSITORY, FORGEJO_TOKEN
# (all injected by Forgejo Actions). GITHUB_API_URL already ends in /api/v1
# (it is the instance's API endpoint), so no version prefix is added here.
set -euo pipefail

if [[ $# -lt 2 || $# -gt 3 ]]; then
    echo "Usage: $(basename "$0") <release-id> <file> [display-name]" >&2
    exit 2
fi

RELEASE_ID="$1"
FILE="$2"
NAME="${3:-$(basename "$FILE")}"

if [[ ! -f "$FILE" ]]; then
    echo "File to upload not found: $FILE" >&2
    exit 1
fi

API="$GITHUB_API_URL/repos/$GITHUB_REPOSITORY/releases/$RELEASE_ID"
AUTH="Authorization: token $FORGEJO_TOKEN"

# Only the first match: a release with two same-named assets (possible only
# through a manual upload) must not produce a malformed DELETE URL.
EXISTING_ID="$(curl -sSf -H "$AUTH" "$API/assets" \
    | jq -r --arg n "$NAME" '([.[] | select(.name == $n)] | .[0].id) // empty')"

if [[ -n "$EXISTING_ID" ]]; then
    echo "Replacing existing asset '$NAME' (id $EXISTING_ID)"
    curl -sSf -H "$AUTH" -X DELETE "$API/assets/$EXISTING_ID" > /dev/null
fi

curl -sSf -H "$AUTH" -X POST -F "name=$NAME" -F "file=@$FILE" "$API/assets" > /dev/null
echo "Uploaded $NAME to release $RELEASE_ID"
