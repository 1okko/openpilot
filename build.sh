#!/usr/bin/bash -e

BUILD_DIR=/data/openpilot
cd $BUILD_DIR
git init
git remote set-url --push origin https://github.com/1okko/openpilot.git

# Cleanup
find . -name '*.a' -delete
find . -name '*.o' -delete
find . -name '*.os' -delete
find . -name '*.pyc' -delete
find . -name 'moc_*' -delete
find . -name '__pycache__' -delete
rm -rf .sconsign.dblite Jenkinsfile release/
rm selfdrive/modeld/models/*.onnx
touch prebuilt

# Add built files to git
git add -f .

VERSION="vw"
git commit -m "vw"

git branch -m  "vw"
git push -f --no-verify origin "vw"
