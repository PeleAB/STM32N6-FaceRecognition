label 0x4e
writefield 0 4 [18] 0x1
burstwrite 23 0 0x8000000 0x15000000
write 23 2 0x70000
burstwriteinc 23 7 0x4000 0x20
burstwriteinc 23 12 0x0 0x10411041
writefield 0 4 [7] 0x1
write 12 0 0x80100
burstwriteinc 12 2 0x342ea000 0x2000
burstwriteinc 12 5 0x0 0x2000 0x0 0x0 0x24 0x100000
burstwrite 12 12 0x6 0x7
burstwriteinc 12 13 0x3 0x342f003f
burstwriteinc 12 17 0x0 0x0
writefield 0 4 [0] 0x1
write 5 0 0x80008
burstwriteinc 5 2 0x342f0000 0x200020 0x180008 0x300 0x8 0x3 0x6000 0x24 0x100000
burstwrite 5 12 0x6 0x7
burstwriteinc 5 13 0x3 0x342f603f
burstwriteinc 5 17 0x0 0x0
write 4 0 0x2
poll 4 0 [1] 0x0 100
write 4 0 0x40000000
poll 4 0 [30] 0x0 100
write 4 0 0x1
write 4 2 0x25
write 4 30 0xf
writefield 5 0 [0] 0x1
writefield 23 0 [0] 0x1
writefield 12 0 [0] 0x1
poll 5 0 [31] 0x0 100
write 4 2 0x0
write 4 30 0x0
write 5 0 0x2
poll 5 0 [1] 0x0 100
write 5 0 0x40000000
poll 5 0 [30] 0x0 100
writefield 0 4 [0] 0x0
write 23 0 0x8000002
poll 23 0 [1] 0x0 100
write 23 0 0x48000000
poll 23 0 [30] 0x0 100
writefield 0 4 [18] 0x0
write 12 0 0x2
poll 12 0 [1] 0x0 100
write 12 0 0x40000000
poll 12 0 [30] 0x0 100
writefield 0 4 [7] 0x0
label 0x4f
writefield 0 4 [12] 0x1
write 17 0 0x100034
burstwriteinc 17 2 0x1180101 0x1100 0x10634101 0x200300 0x2ff0000 0x1f0000
write 17 10 0x0
write 17 13 0x0
writefield 0 4 [19] 0x1
burstwrite 24 0 0x8000000 0x9000060
write 24 2 0x88000
burstwriteinc 24 7 0x7d14 0x0
burstwriteinc 24 12 0x0 0x20422042
writefield 0 4 [20] 0x1
burstwrite 25 0 0x8000000 0x15000060
write 25 2 0xa0000
burstwriteinc 25 7 0xa762663e 0xfd43
burstwriteinc 25 12 0x0 0x20422042
writefield 0 4 [17] 0x1
write 22 0 0x174104c
burstwriteinc 22 2 0x87f80 0x10320f
write 22 256 0xc0
burstwriteinc 22 512 0xf220089 0xda9e 0xeae00b4 0xd371 0x6150054 0xc7f6 0xb4d0104 0xcaa2 0xb560238 0xcae0 0x16080145 0xc39c 0x4600fe39 0x9427 0x4900fe00 0x9331
writefield 0 4 [4] 0x1
write 9 0 0x80104
burstwriteinc 9 2 0x342f0000 0x6000
burstwriteinc 9 5 0x0 0x6000 0x1 0x0 0x24 0x100000
burstwrite 9 12 0x6 0x7
burstwriteinc 9 13 0x1 0x342f603f
burstwriteinc 9 17 0x0 0x0
writefield 0 4 [7] 0x1
write 12 0 0x80184
burstwriteinc 12 2 0x712576c0 0x18
burstwriteinc 12 5 0x0 0x0 0x0 0x0 0x24 0x100000
burstwrite 12 12 0x6 0x7
burstwriteinc 12 13 0x1 0x71257717
burstwriteinc 12 17 0x0 0x8
writefield 0 4 [6] 0x1
write 11 0 0x8010c
burstwriteinc 11 2 0x342eb000 0x400
burstwriteinc 11 5 0x0 0x400 0x0 0x0 0x24 0x100000
burstwrite 11 12 0x6 0x7
burstwriteinc 11 13 0x1 0x342eb43f
burstwriteinc 11 17 0x0 0x0
write 4 0 0x2
poll 4 0 [1] 0x0 100
write 4 0 0x40000000
poll 4 0 [30] 0x0 100
write 4 0 0x1
write 4 8 0x23
burstwriteinc 4 18 0x9 0xf
write 4 29 0x29
write 4 32 0x19
write 4 34 0x27
writefield 11 0 [0] 0x1
write 17 0 0x100035
writefield 22 0 [0] 0x1
writefield 24 0 [0] 0x1
writefield 25 0 [0] 0x1
writefield 9 0 [0] 0x1
writefield 12 0 [0] 0x1
poll 11 0 [31] 0x0 100
write 4 8 0x0
burstwriteinc 4 18 0x0 0x0
write 4 29 0x0
write 4 32 0x0
write 4 34 0x0
write 11 0 0x2
poll 11 0 [1] 0x0 100
write 11 0 0x40000000
poll 11 0 [30] 0x0 100
writefield 0 4 [6] 0x0
write 17 0 0x2
poll 17 0 [1] 0x0 100
write 17 0 0x40000000
poll 17 0 [30] 0x0 100
writefield 0 4 [12] 0x0
write 22 0 0x881082
poll 22 0 [1] 0x0 100
write 22 0 0x40881080
poll 22 0 [30] 0x0 100
writefield 0 4 [17] 0x0
write 24 0 0x8000002
poll 24 0 [1] 0x0 100
write 24 0 0x48000000
poll 24 0 [30] 0x0 100
writefield 0 4 [19] 0x0
write 25 0 0x8000002
poll 25 0 [1] 0x0 100
write 25 0 0x48000000
poll 25 0 [30] 0x0 100
writefield 0 4 [20] 0x0
write 9 0 0x2
poll 9 0 [1] 0x0 100
write 9 0 0x40000000
poll 9 0 [30] 0x0 100
writefield 0 4 [4] 0x0
write 12 0 0x2
poll 12 0 [1] 0x0 100
write 12 0 0x40000000
poll 12 0 [30] 0x0 100
writefield 0 4 [7] 0x0
irq 0x0
