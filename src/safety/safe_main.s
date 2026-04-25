.syntax unified
.cpu cortex-m3
.fpu softvfp
.thumb

.global g_pfnVectors
.global Reset_Handler

/* 栈顶地址 (SRAM 20KB: 0x20000000 + 0x5000) */
.word 0x20005000
.word Reset_Handler

.section .text.Reset_Handler
Reset_Handler:
  b Reset_Handler
