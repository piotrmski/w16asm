; This program takes characters from terminal input and echoes them back capitalized until Return is pressed.

loop:
ld IO
st character
jmz loop

; if character == '\n' (10) then end
add constNegativeLF
jmz onReturn
add constLF

; if not character < 'a' (97) and character < 'z'+1 (123) then print character - 32; else print character
add constNegativea
jmn echo
add consta 

add constNegativezMinus1
jmn capitalize
add constzPlus1 

echo:
ld character
st IO
jmp loop

capitalize:
ld character
add constNegative32
st IO
jmp loop

onReturn:
ld character
st IO

end: jmp end

character:            ' '
const1:               1
constNegativeLF:      -'\n'
constLF:              '\n'
constNegativea:       -'a'
consta:               'a'
constNegativezMinus1: -'z'-1
constzPlus1:          'z'+1
constNegative32:      -0x20

IO: .org 0x1fff