#!/bin/bash

# pega última tag
LAST_TAG=$(git describe --tags --abbrev=0 2>/dev/null)

# se não existir
if [ -z "$LAST_TAG" ]; then
  NEW_TAG="v1.0.0"
else
  # incrementa PATCH
  VERSION=${LAST_TAG#v}
  IFS='.' read -r MAJOR MINOR PATCH <<< "$VERSION"
  PATCH=$((PATCH + 1))
  NEW_TAG="v$MAJOR.$MINOR.$PATCH"
fi

echo "Nova versão: $NEW_TAG"

# commit automático (opcional)
git add .
git commit -m "release: $NEW_TAG"

# cria tag
git tag $NEW_TAG

# push
git push origin main
git push origin $NEW_TAG