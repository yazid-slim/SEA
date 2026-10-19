#!/bin/sh
set -eu

demo_output=$(./battleship --demo 4 42)
printf '%s\n' "$demo_output" | grep -q 'gagne !'

eof_output=$(./battleship 2 42 </dev/null)
printf '%s\n' "$eof_output" | grep -q 'partie arrêtée proprement'

invalid_output=$(printf 'abc\n1 1 1\n2 11 1\n' | ./battleship 2 42)
printf '%s\n' "$invalid_output" | grep -q 'Utilisez trois nombres'
printf '%s\n' "$invalid_output" | grep -q 'Tir invalide'

repeat_output=$(printf '2 1 3\n1 1 1\n2 1 3\n' | ./battleship 2 42)
printf '%s\n' "$repeat_output" | grep -q 'déjà été visée'

echo 'Integration tests passed.'
