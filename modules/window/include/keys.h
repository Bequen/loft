#define KEY_BackSpace                     0xff08  /* Back space, back char */
#define KEY_Tab                           0xff09
#define KEY_Linefeed                      0xff0a  /* Linefeed, LF */
#define KEY_Clear                         0xff0b
#define KEY_Return                        0xff0d  /* Return, enter */
#define KEY_Pause                         0xff13  /* Pause, hold */
#define KEY_Scroll_Lock                   0xff14
#define KEY_Sys_Req                       0xff15
#define KEY_Escape                        0xff1b
#define KEY_Delete                        0xffff  /* Delete, rubout */



/* Cursor control & motion */

#define KEY_Home                          0xff50
#define KEY_Left                          0xff51  /* Move left, left arrow */
#define KEY_Up                            0xff52  /* Move up, up arrow */
#define KEY_Right                         0xff53  /* Move right, right arrow */
#define KEY_Down                          0xff54  /* Move down, down arrow */
#define KEY_Prior                         0xff55  /* Prior, previous */
#define KEY_Page_Up                       0xff55
#define KEY_Next                          0xff56  /* Next */
#define KEY_Page_Down                     0xff56
#define KEY_End                           0xff57  /* EOL */
#define KEY_Begin                         0xff58  /* BOL */


/* Misc functions */

#define KEY_Select                        0xff60  /* Select, mark */
#define KEY_Print                         0xff61
#define KEY_Execute                       0xff62  /* Execute, run, do */
#define KEY_Insert                        0xff63  /* Insert, insert here */
#define KEY_Undo                          0xff65
#define KEY_Redo                          0xff66  /* Redo, again */
#define KEY_Menu                          0xff67
#define KEY_Find                          0xff68  /* Find, search */
#define KEY_Cancel                        0xff69  /* Cancel, stop, abort, exit */
#define KEY_Help                          0xff6a  /* Help */
#define KEY_Break                         0xff6b
#define KEY_Mode_switch                   0xff7e  /* Character set switch */
#define KEY_script_switch                 0xff7e  /* Alias for mode_switch */
#define KEY_Num_Lock                      0xff7f

/* Modifiers */

#define KEY_Shift_L                       0xffe1  /* Left shift */
#define KEY_Shift_R                       0xffe2  /* Right shift */
#define KEY_Control_L                     0xffe3  /* Left control */
#define KEY_Control_R                     0xffe4  /* Right control */
#define KEY_Caps_Lock                     0xffe5  /* Caps lock */
#define KEY_Shift_Lock                    0xffe6  /* Shift lock */

#define KEY_Meta_L                        0xffe7  /* Left meta */
#define KEY_Meta_R                        0xffe8  /* Right meta */
#define KEY_Alt_L                         0xffe9  /* Left alt */
#define KEY_Alt_R                         0xffea  /* Right alt */
#define KEY_Super_L                       0xffeb  /* Left super */
#define KEY_Super_R                       0xffec  /* Right super */
#define KEY_Hyper_L                       0xffed  /* Left hyper */
#define KEY_Hyper_R                       0xffee  /* Right hyper */


/* Keypad functions, keypad numbers cleverly chosen to map to ASCII */

#define KEY_KP_Space                      0xff80  /* Space */
#define KEY_KP_Tab                        0xff89
#define KEY_KP_Enter                      0xff8d  /* Enter */
#define KEY_KP_F1                         0xff91  /* PF1, KP_A, ... */
#define KEY_KP_F2                         0xff92
#define KEY_KP_F3                         0xff93
#define KEY_KP_F4                         0xff94
#define KEY_KP_Home                       0xff95
#define KEY_KP_Left                       0xff96
#define KEY_KP_Up                         0xff97
#define KEY_KP_Right                      0xff98
#define KEY_KP_Down                       0xff99
#define KEY_KP_Prior                      0xff9a
#define KEY_KP_Page_Up                    0xff9a
#define KEY_KP_Next                       0xff9b
#define KEY_KP_Page_Down                  0xff9b
#define KEY_KP_End                        0xff9c
#define KEY_KP_Begin                      0xff9d
#define KEY_KP_Insert                     0xff9e
#define KEY_KP_Delete                     0xff9f
#define KEY_KP_Equal                      0xffbd  /* Equals */
#define KEY_KP_Multiply                   0xffaa
#define KEY_KP_Add                        0xffab
#define KEY_KP_Separator                  0xffac  /* Separator, often comma */
#define KEY_KP_Subtract                   0xffad
#define KEY_KP_Decimal                    0xffae
#define KEY_KP_Divide                     0xffaf

#define KEY_KP_0                          0xffb0
#define KEY_KP_1                          0xffb1
#define KEY_KP_2                          0xffb2
#define KEY_KP_3                          0xffb3
#define KEY_KP_4                          0xffb4
#define KEY_KP_5                          0xffb5
#define KEY_KP_6                          0xffb6
#define KEY_KP_7                          0xffb7
#define KEY_KP_8                          0xffb8
#define KEY_KP_9                          0xffb9



/*
 * Auxiliary functions; note the duplicate definitions for left and right
 * function keys;  Sun keyboards and a few other manufacturers have such
 * function key groups on the left and/or right sides of the keyboard.
 * We've not found a keyboard with more than 35 function keys total.
 */

#define KEY_F1                            0xffbe
#define KEY_F2                            0xffbf
#define KEY_F3                            0xffc0
#define KEY_F4                            0xffc1
#define KEY_F5                            0xffc2
#define KEY_F6                            0xffc3
#define KEY_F7                            0xffc4
#define KEY_F8                            0xffc5
#define KEY_F9                            0xffc6
#define KEY_F10                           0xffc7
#define KEY_F11                           0xffc8
#define KEY_L1                            0xffc8
#define KEY_F12                           0xffc9
#define KEY_L2                            0xffc9
#define KEY_F13                           0xffca
#define KEY_L3                            0xffca
#define KEY_F14                           0xffcb
#define KEY_L4                            0xffcb
#define KEY_F15                           0xffcc
#define KEY_L5                            0xffcc
#define KEY_F16                           0xffcd
#define KEY_L6                            0xffcd
#define KEY_F17                           0xffce
#define KEY_L7                            0xffce
#define KEY_F18                           0xffcf
#define KEY_L8                            0xffcf
#define KEY_F19                           0xffd0
#define KEY_L9                            0xffd0
#define KEY_F20                           0xffd1
#define KEY_L10                           0xffd1
#define KEY_F21                           0xffd2
#define KEY_R1                            0xffd2
#define KEY_F22                           0xffd3
#define KEY_R2                            0xffd3
#define KEY_F23                           0xffd4
#define KEY_R3                            0xffd4
#define KEY_F24                           0xffd5
#define KEY_R4                            0xffd5
#define KEY_F25                           0xffd6
#define KEY_R5                            0xffd6
#define KEY_F26                           0xffd7
#define KEY_R6                            0xffd7
#define KEY_F27                           0xffd8
#define KEY_R7                            0xffd8
#define KEY_F28                           0xffd9
#define KEY_R8                            0xffd9
#define KEY_F29                           0xffda
#define KEY_R9                            0xffda
#define KEY_F30                           0xffdb
#define KEY_R10                           0xffdb
#define KEY_F31                           0xffdc
#define KEY_R11                           0xffdc
#define KEY_F32                           0xffdd
#define KEY_R12                           0xffdd
#define KEY_F33                           0xffde
#define KEY_R13                           0xffde
#define KEY_F34                           0xffdf
#define KEY_R14                           0xffdf
#define KEY_F35                           0xffe0
#define KEY_R15                           0xffe0

#define KEY_space                         0x0020  /* U+0020 SPACE */
#define KEY_exclam                        0x0021  /* U+0021 EXCLAMATION MARK */
#define KEY_quotedbl                      0x0022  /* U+0022 QUOTATION MARK */
#define KEY_numbersign                    0x0023  /* U+0023 NUMBER SIGN */
#define KEY_dollar                        0x0024  /* U+0024 DOLLAR SIGN */
#define KEY_percent                       0x0025  /* U+0025 PERCENT SIGN */
#define KEY_ampersand                     0x0026  /* U+0026 AMPERSAND */
#define KEY_apostrophe                    0x0027  /* U+0027 APOSTROPHE */
#define KEY_quoteright                    0x0027  /* deprecated */
#define KEY_parenleft                     0x0028  /* U+0028 LEFT PARENTHESIS */
#define KEY_parenright                    0x0029  /* U+0029 RIGHT PARENTHESIS */
#define KEY_asterisk                      0x002a  /* U+002A ASTERISK */
#define KEY_plus                          0x002b  /* U+002B PLUS SIGN */
#define KEY_comma                         0x002c  /* U+002C COMMA */
#define KEY_minus                         0x002d  /* U+002D HYPHEN-MINUS */
#define KEY_period                        0x002e  /* U+002E FULL STOP */
#define KEY_slash                         0x002f  /* U+002F SOLIDUS */
#define KEY_0                             0x0030  /* U+0030 DIGIT ZERO */
#define KEY_1                             0x0031  /* U+0031 DIGIT ONE */
#define KEY_2                             0x0032  /* U+0032 DIGIT TWO */
#define KEY_3                             0x0033  /* U+0033 DIGIT THREE */
#define KEY_4                             0x0034  /* U+0034 DIGIT FOUR */
#define KEY_5                             0x0035  /* U+0035 DIGIT FIVE */
#define KEY_6                             0x0036  /* U+0036 DIGIT SIX */
#define KEY_7                             0x0037  /* U+0037 DIGIT SEVEN */
#define KEY_8                             0x0038  /* U+0038 DIGIT EIGHT */
#define KEY_9                             0x0039  /* U+0039 DIGIT NINE */
#define KEY_colon                         0x003a  /* U+003A COLON */
#define KEY_semicolon                     0x003b  /* U+003B SEMICOLON */
#define KEY_less                          0x003c  /* U+003C LESS-THAN SIGN */
#define KEY_equal                         0x003d  /* U+003D EQUALS SIGN */
#define KEY_greater                       0x003e  /* U+003E GREATER-THAN SIGN */
#define KEY_question                      0x003f  /* U+003F QUESTION MARK */
#define KEY_at                            0x0040  /* U+0040 COMMERCIAL AT */
#define KEY_A                             0x0041  /* U+0041 LATIN CAPITAL LETTER A */
#define KEY_B                             0x0042  /* U+0042 LATIN CAPITAL LETTER B */
#define KEY_C                             0x0043  /* U+0043 LATIN CAPITAL LETTER C */
#define KEY_D                             0x0044  /* U+0044 LATIN CAPITAL LETTER D */
#define KEY_E                             0x0045  /* U+0045 LATIN CAPITAL LETTER E */
#define KEY_F                             0x0046  /* U+0046 LATIN CAPITAL LETTER F */
#define KEY_G                             0x0047  /* U+0047 LATIN CAPITAL LETTER G */
#define KEY_H                             0x0048  /* U+0048 LATIN CAPITAL LETTER H */
#define KEY_I                             0x0049  /* U+0049 LATIN CAPITAL LETTER I */
#define KEY_J                             0x004a  /* U+004A LATIN CAPITAL LETTER J */
#define KEY_K                             0x004b  /* U+004B LATIN CAPITAL LETTER K */
#define KEY_L                             0x004c  /* U+004C LATIN CAPITAL LETTER L */
#define KEY_M                             0x004d  /* U+004D LATIN CAPITAL LETTER M */
#define KEY_N                             0x004e  /* U+004E LATIN CAPITAL LETTER N */
#define KEY_O                             0x004f  /* U+004F LATIN CAPITAL LETTER O */
#define KEY_P                             0x0050  /* U+0050 LATIN CAPITAL LETTER P */
#define KEY_Q                             0x0051  /* U+0051 LATIN CAPITAL LETTER Q */
#define KEY_R                             0x0052  /* U+0052 LATIN CAPITAL LETTER R */
#define KEY_S                             0x0053  /* U+0053 LATIN CAPITAL LETTER S */
#define KEY_T                             0x0054  /* U+0054 LATIN CAPITAL LETTER T */
#define KEY_U                             0x0055  /* U+0055 LATIN CAPITAL LETTER U */
#define KEY_V                             0x0056  /* U+0056 LATIN CAPITAL LETTER V */
#define KEY_W                             0x0057  /* U+0057 LATIN CAPITAL LETTER W */
#define KEY_X                             0x0058  /* U+0058 LATIN CAPITAL LETTER X */
#define KEY_Y                             0x0059  /* U+0059 LATIN CAPITAL LETTER Y */
#define KEY_Z                             0x005a  /* U+005A LATIN CAPITAL LETTER Z */
#define KEY_bracketleft                   0x005b  /* U+005B LEFT SQUARE BRACKET */
#define KEY_backslash                     0x005c  /* U+005C REVERSE SOLIDUS */
#define KEY_bracketright                  0x005d  /* U+005D RIGHT SQUARE BRACKET */
#define KEY_asciicircum                   0x005e  /* U+005E CIRCUMFLEX ACCENT */
#define KEY_underscore                    0x005f  /* U+005F LOW LINE */
#define KEY_grave                         0x0060  /* U+0060 GRAVE ACCENT */
#define KEY_quoteleft                     0x0060  /* deprecated */
#define KEY_a                             0x0061  /* U+0061 LATIN SMALL LETTER A */
#define KEY_b                             0x0062  /* U+0062 LATIN SMALL LETTER B */
#define KEY_c                             0x0063  /* U+0063 LATIN SMALL LETTER C */
#define KEY_d                             0x0064  /* U+0064 LATIN SMALL LETTER D */
#define KEY_e                             0x0065  /* U+0065 LATIN SMALL LETTER E */
#define KEY_f                             0x0066  /* U+0066 LATIN SMALL LETTER F */
#define KEY_g                             0x0067  /* U+0067 LATIN SMALL LETTER G */
#define KEY_h                             0x0068  /* U+0068 LATIN SMALL LETTER H */
#define KEY_i                             0x0069  /* U+0069 LATIN SMALL LETTER I */
#define KEY_j                             0x006a  /* U+006A LATIN SMALL LETTER J */
#define KEY_k                             0x006b  /* U+006B LATIN SMALL LETTER K */
#define KEY_l                             0x006c  /* U+006C LATIN SMALL LETTER L */
#define KEY_m                             0x006d  /* U+006D LATIN SMALL LETTER M */
#define KEY_n                             0x006e  /* U+006E LATIN SMALL LETTER N */
#define KEY_o                             0x006f  /* U+006F LATIN SMALL LETTER O */
#define KEY_p                             0x0070  /* U+0070 LATIN SMALL LETTER P */
#define KEY_q                             0x0071  /* U+0071 LATIN SMALL LETTER Q */
#define KEY_r                             0x0072  /* U+0072 LATIN SMALL LETTER R */
#define KEY_s                             0x0073  /* U+0073 LATIN SMALL LETTER S */
#define KEY_t                             0x0074  /* U+0074 LATIN SMALL LETTER T */
#define KEY_u                             0x0075  /* U+0075 LATIN SMALL LETTER U */
#define KEY_v                             0x0076  /* U+0076 LATIN SMALL LETTER V */
#define KEY_w                             0x0077  /* U+0077 LATIN SMALL LETTER W */
#define KEY_x                             0x0078  /* U+0078 LATIN SMALL LETTER X */
#define KEY_y                             0x0079  /* U+0079 LATIN SMALL LETTER Y */
#define KEY_z                             0x007a  /* U+007A LATIN SMALL LETTER Z */
#define KEY_braceleft                     0x007b  /* U+007B LEFT CURLY BRACKET */
#define KEY_bar                           0x007c  /* U+007C VERTICAL LINE */
#define KEY_braceright                    0x007d  /* U+007D RIGHT CURLY BRACKET */
#define KEY_asciitilde                    0x007e  /* U+007E TILDE */

