Dear TA,

For any user, my program functions under certain circumstances.

If you get to the line 
	Valid commands for TA: (for the TA) OR 
	Valid courses: CSC108, CSC148, CSC209 Which course are you asking about? (for the student)
then it should work fine.

Some times because of unhandled buffer overflow users may not be correctly initialized and the program doesn't correctly.

Another point is that once you get to the aforemention lines successfully you may get syntax errors even if you
enter correct in put. This again is due to unhandled buffer overflow.

Moral: 
Please try a few times to initialize two each of TA and Student without error as described above and you can suscessful check the
next and/or stats commands.



Welcome to the Help Centre, what is your name?
T1
Are you are a TA or Student(enter T or S)
Invalid (enter T or S)
T^C
wolf:~$  nc -C localhost 53691
Welcome to the Help Centre, what is your name?
t1
Are you are a TA or Student(enter T or S)
T
Valid commands for TA:
        stats
        next
        (or use Ctrl-C to leave)
stats
Full Queue

Incorrect syntax2
