; ---------------------------------------------------------------------------
; crt0.s
; ---------------------------------------------------------------------------
;
; Startup code for cc65 (Single Board Computer version)

.export   _init, _exit
.import   _main

.export   __STARTUP__ : absolute = 1        ; Mark as startup

.segment  "CODE"

_init:    LDX     #$FF                 ; Initialize stack pointer to $01FF
          TXS

          JSR     _main
_exit:    
          BRK

