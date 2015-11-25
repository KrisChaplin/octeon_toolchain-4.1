! Generated automatically from TIPS by tips2m4 -- DO NOT EDIT
include(ifdef(`macros',macros(),ifdef(`srcdir',srcdir()/macros.m4,macros.m4)))
! Initial tip (number 0) - shown after wrap-around.

Ddd*tip0: \
@rm If you have any more DDD tips of the day,\n\
please send them to the DDD developers EMAIL([ddd]@ gnu.org).


! First tips.

Ddd*tip1: \
@rm Welcome to DDD!\n\
You can get help on all items by pointing at them and pressing F1.\n\
And don't worry, you can undo most mistakes...

Ddd*tip2: \
@rm Whenever you're stuck, try LBL(Help, What Now).  This will analyze the\n\
current DDD state and give you some hints on what to do next.

Ddd*tip3: \
@rm Buttons with a small triangle in the upper right corner are special:\n\
pressing and holding BUTTON(1) on them will pop up a menu \n\
with additional options.

Ddd*tip4: \
@rm You can interrupt @GDB@ and the current program by clicking on\n\
LBL(Program, Interrupt) or pressing KEY(Esc).

Ddd*tip5: \
@rm If you made a mistake, try LBL(Edit, Undo).  This will undo the most\n\
recent debugger command and redisplay the previous program state.

Ddd*tip6: \
@rm There are three ways to show the value of a variable:\n\
ITEM You can view its value, simply by pointing at it;\n\
ITEM You can print its value in the debugger console, using LBL(Print ());\n\
ITEM You can display it graphically, using LBL(Display ()).

Ddd*tip7: \
@rm A quick way to manipulate variables, breakpoints, and displays\n\
is to press BUTTON(3) on them.

Ddd*tip8: \
@rm Double-clicking on any value in the data display \n\
will toggle more details.

Ddd*tip9: \
@rm If your program needs special terminal capabilities such as readline\n\
or curses, let it run in the separate execution window \n\
(LBL(Program, Run in Execution Window)).


! Intermediate tips.

Ddd*tip10: \
@rm You can save space in the data window by \n\
ITEM hiding details (LBL(Show/Hide)),\n\
ITEM rotating structs (LBL(Rotate)), and \n\
ITEM clustering variables (LBL(Undisp, Cluster)).

Ddd*tip11: \
@rm If you want to customize the DDD fonts, see\n\
LBL(Edit, Preferences, Fonts).

Ddd*tip12: \
@rm To change the text background color, write into FILE(~/.[ddd]/init):\n\
CODE([Ddd]*XmText.background:      ) VAR(color)\n\
CODE([Ddd]*XmTextField.background: ) VAR(color)\n\
CODE([Ddd]*XmList.background:      ) VAR(color)

Ddd*tip13: \
@rm When using GDB, all text fields have command and argument completion\n\
with KEY(Tab), just like the shell.  In a file selection box, type part of\n\
a filename, hit KEY(Tab), and voila!  It's completed.

Ddd*tip14: \
@rm You can always recenter the command tool\n\
by selecting LBL(View, Command Tool),\n\
or by pressing KEY(Alt+8).

Ddd*tip15: \
@rm To scroll the data display, most users find a EMPH(panner)\n\
much more convenient than two scrollbars.\n\
Check out LBL(Edit, Preferences, Startup, Data Scrolling).

Ddd*tip16: \
@rm To limit the number of array elements in a data display, set\n\
LBL(Edit, GDB Settings, Limit on array elements to print).

Ddd*tip17: \
@rm If DDD cannot find a source, set the @GDB@ source path via \n\
LBL(Edit, @GDB@ Settings) or enter \n\
SAMP(dir sourcedir_1:sourcedir_2:...:sourcedir_n) at the GDB prompt.

Ddd*tip18: \
@rm You can quickly set breakpoints\n\
by double-clicking in the breakpoint area.

Ddd*tip19: \
@rm To see the EMPH(actual) type of a C++ object in GDB, set\n\
LBL(Edit, GDB Settings, Set printing of object's derived type).

Ddd*tip20: \
@rm To display VAR(data) in hexadecimal format, display it and choose\n\
LBL(Convert to Hex) from the LBL(Display ()) menu, \n\
or enter KBD(graph display /x VAR(data)) at the GDB prompt.

Ddd*tip21: \
@rm To send a signal to your program,\n\
use LBL(Status, Signals, Send).

Ddd*tip22: \
@rm To quickly display variable values,\n\
double-click on the variable name.

Ddd*tip23: \
@rm After looking up an item or stepping through the program, you can use\n\
LBL(Edit, Undo) and LBL(Edit, Redo) to return to earlier locations.

Ddd*tip24: \
@rm You can repeat the last command by hitting KEY_RETURN.\n\
Use KEY(Ctrl+B) and KEY(Ctrl+F) to search the command history.

Ddd*tip25: \
@rm You can move breakpoints by dragging them.  \n\
Just press and hold BUTTON(1) on a breakpoint,\n\
move it to the new position and release BUTTON(1) again.

Ddd*tip26: \
@rm To make your program ignore signals, use LBL(Status, Signals) and\n\
unset the LBL(Pass) button for the appropriate signal.

Ddd*tip27: \
@rm You can save space by disabling toolbar captions.\n\
See LBL(Edit, Preferences, Startup, Toolbar Appearance).

Ddd*tip28: \
@rm To quickly edit breakpoint properties, \n\
double-click on a breakpoint symbol.

Ddd*tip29: \
@rm To have GDB start your program automatically upon startup,\n\
put the following lines in your FILE(.gdbinit) file:\n\
CODE(break main)      - or some other initial function\n\
CODE(run       )      - possibly giving arguments here


! Advanced tips.

Ddd*tip30: \
@rm To get rid of these tips of the day, unset\n\
LBL(Edit, Preferences, Startup, Show Tip of the Day).

Ddd*tip31: \
@rm To redirect stderr from the execution window to the debugger console,\n\
add SAMP(2>/dev/tty) to the arguments of your program.

Ddd*tip32: \
@rm To display the first VAR(n) elements of a variable-length array VAR(ptr),\n\
enter KBD(graph display VAR(ptr)\1330\135@ VAR(n)) at the GDB prompt.

Ddd*tip33: \
@rm To undisplay a specific record member once and for all, select it in a\n\
display and click on LBL(Undisp).  Confirming with LBL(Apply to All)\n\
suppresses the member in other displays, too.

Ddd*tip34: \
@rm To display data in a smaller font, try LBL(Data, Themes, Small Values).\n\
The pattern describes the expressions you want to see in smaller font:\n\
SAMP(*) matches all expressions.

Ddd*tip35: \
@rm You can have each of DDD, @GDB@ and the debugged program run on\n\
different machines.  See the DDD KBD(--rhost) option for details.

Ddd*tip36: \
@rm You can copy breakpoints by dragging them while pressing KEY(Shift).\n\
Just press and hold KEY(Shift)+BUTTON(1) on a breakpoint,\n\
move it to the new position and release BUTTON(1) again.

Ddd*tip37: \
@rm To save and restore data displays, cut, copy and paste them via\n\
the LBL(Edit) menu.  Together with CODE(xclipboard), you can manage\n\
arbitrary collections of data displays.

Ddd*tip38: \
@rm Do you want to stop this debugging session and resume later?\n\
Use LBL(File, Save Session)!

Ddd*tip39: \
@rm You can interact with DDD even while the debuggee is executing:\n\
DDD automatically interrupts program execution for a moment.  \n\
This works whenever program execution is initiated by LBL(Run) or LBL(Cont).

Ddd*tip40: \
@rm To debug a child process, put a call to SAMP(sleep) in the child right\n\
after the SAMP(fork) call.  Run the program and attach to the child process\n\
using LBL(File, Attach to Process).

Ddd*tip41: \
@rm If your program prints a lot of text on standard error, try\n\
redirecting standard error to a file (via SAMP(2> VAR(FILE))), or add\n\
to FILE(~/.[ddd]/init): CODE([Ddd]*lineBufferedConsole: off).

Ddd*tip42: \
@rm If the inferior debugger does not support stderr redirection, try\n\
invoking DDD using KBD(--debugger 'VAR(NAME) 2> VAR(FILE)').

Ddd*tip43: \
@rm Using GDB, you can define your own canned sequences of commands.\n\
Try LBL(Commands, Define Command).

Ddd*tip44: \
@rm To use GDB with Solaris CC, compile with SAMP(-xs).\n\
GDB wants debugging info in the executable.

Ddd*tip45: \
@rm To use GDB with G77, compile with SAMP(-fdebug-kludge).  This gives\n\
rudimentary information on COMMON and EQUIVALENCE variables in GDB.\n\
See the G77 documentation for details.

Ddd*tip46: \
@rm Double-clicking on a function call will lead you to the definition of\n\
the function.  Use LBL(Edit, Undo) to return to the function call.

Ddd*tip47: \
@rm Disabled breakpoints can be used as bookmarks.\n\
Use LBL(Source, Edit Breakpoints) to list all breakpoints;\n\
then, click on LBL(Lookup) to jump to a breakpoint location.

Ddd*tip48: \
@rm You can assign user-defined buttons to frequently used commands.\n\
Try LBL(Commands, Edit Buttons).

Ddd*tip49: \
@rm In the Breakpoint and Display Editors,\n\
you can toggle the selection with KEY(Ctrl)+BUTTON(1).\n\
This allows you to select non-contiguous ranges of items.

Ddd*tip50: \
@rm To change the properties of multiple breakpoints at once,\n\
select them in the breakpoint editor (LBL(Source, Edit Breakpoints))\n\
and click on LBL(Properties).

Ddd*tip51: \
@rm Even while in the source window, \n\
you can enter and edit GDB commands:\n\
Just type the command and press KEY_RETURN.

Ddd*tip52: \
@rm You can record commands to be executed when a breakpoint is hit.\n\
In the LBL(Breakpoint Properties) panel, try LBL(Record) and LBL(End).

Ddd*tip53: \
@rm You can easily resume a saved DDD VAR(session)\n\
by invoking DDD as KBD([ddd] =VAR(session)).

Ddd*tip54: \
@rm If the DDD source window keeps on scrolling until the end of source is\n\
reached, try changing the SAMP(glyphUpdateDelay) resource.  See the\n\
DDD manual for details.

Ddd*tip55: \
@rm To customize display appearance, you can write your own themes.\n\
For details, see the  manual "Writing DDD Themes".

Ddd*tip56: \
@rm For further DDD customization (e.g. colors), see the FILE([Ddd])\n\
app-defaults file from the DDD WWW Site or the DDD source\n\
distribution.  Copy resources to FILE(~/.[ddd]/init) and edit as desired.


! Tips from professionals.

Ddd*tip57: \
@rm In GCC, SAMP(-Wall) does not enable all warnings.  See the GCC\n\
documentation for other warnings you might consider useful.\n\
BY(J.H.M. Dassen, jdassen@ wi.leidenuniv.nl)

Ddd*tip58: \
@rm To get a global idea of what a process or program does or is doing,\n\
use a system call tracer, like SAMP(strace), SAMP(truss), or SAMP(trace).\n\
BY(J.H.M. Dassen, jdassen@ wi.leidenuniv.nl)

Ddd*tip59: \
@rm If you suspect memory corruption caused by problematic pointers, \n\
try linking with Electric Fence SAMP(efence) or the SAMP(dbmalloc) library.\n\
See the DDD WWW page for links.\n\
BY(J.H.M. Dassen, jdassen@ wi.leidenuniv.nl)

Ddd*tip60: \
@rm Prevention is better than cure.  Document your assumptions using\n\
CODE(<assert.h>) or GNU NANA.  See the DDD WWW page for links.\n\
BY(J.H.M. Dassen, jdassen@ wi.leidenuniv.nl)

Ddd*tip61: \
@rm The debugger isn't a substitute for good thinking.  But, in some\n\
cases, thinking isn't a substitute for a good debugger either.  The\n\
most effective combination is good thinking and a good debugger.\n\
QUOTE(Steve McConnell, Code Complete)

Ddd*tip62: \
@rm When you're totally stuck, try to find a helpful ear.  In my experience,\n\
nothing helps you more in debugging than to try to explain your problem\n\
(what your code should do, and what it actually does) to someone else.\n\
BY(J.H.M. Dassen, jdassen@ wi.leidenuniv.nl)

! If you have other questions, comments or suggestions, contact The King via
! electronic mail to EMAIL(elvis@ graceland.gnu.org).  The King will
! try to help you out, although he may not have time to fix your problems.
! QUOTE(Free Software Foundation, GNU Hello Manual)

