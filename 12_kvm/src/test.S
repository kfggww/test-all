mov $0x3f8, %dx
add %bl, %al
add $'a', %al
out %al, (%dx)
mov $'\n', %al
out %al, (%dx)
hlt