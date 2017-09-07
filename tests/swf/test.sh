APP=../../build/lvg_test
if [ ! -f "$APP" ]; then
    mkdir ../../build
    pushd ../../build
    meson -Dbuildtype=debug -Db_sanitize=address ..
    ninja
    popd
fi

num_fail=0
num_pass=0
for i in trace/*.swf; do
tput ed
echo -ne "test $i (${num_pass} passed, ${num_fail} failed)\r"
$($APP $i >$i.failed 2>&1)
if [ ! $? -eq 0 ]; then
    tput ed
    echo $i exited with non-zero error code
fi
cmp -s $i.failed $i.trace > /dev/null
if [ $? -eq 1 ]; then
    num_fail=$((num_fail + 1))
else
    rm $i.failed
    num_pass=$((num_pass + 1))
fi
done

echo passed tests: ${num_pass}
echo failed tests: ${num_fail}
