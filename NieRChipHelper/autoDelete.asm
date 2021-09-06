extern AUTODELETE_SKIP:ptr
extern AUTODELETE_JUMPBACK:ptr

.code

AUTODELETE PROC

  cmp r14, 0h
  je rank0
  cmp r14, 1h
  je rank1
  cmp r14, 2h
  je rank2
  cmp r14, 3h
  je rank3
  cmp r14, 4h
  je rank4
  cmp r14, 5h
  je rank5
  cmp r14, 6h
  je rank6
  cmp r14, 7h
  je rank7

  ; level 8 or an hell of a strange case
  jmp continue

rank0:
  cmp r12, 6
  jle continue
  jmp useless_chip
rank1:
  cmp r12, 7
  jle continue
  jmp useless_chip
rank2:
  cmp r12, 8
  jle continue
  jmp useless_chip
rank3:
  cmp r12, 9
  jle continue
  jmp useless_chip
rank4:
  cmp r12, 11
  jle continue
  jmp useless_chip
rank5:
  cmp r12, 13
  jle continue
  jmp useless_chip
rank6:
  cmp r12, 16
  jle continue
  jmp useless_chip
rank7:
  cmp r12, 19
  jle continue
  jmp useless_chip

useless_chip:
  mov EBX, 0FFFFFFFFh
  jmp [AUTODELETE_SKIP]

continue:
  lea eax,[rsi-00000BB9h]
  jmp [AUTODELETE_JUMPBACK]

AUTODELETE ENDP

PUBLIC AUTODELETE
END