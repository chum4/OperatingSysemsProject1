# OperatingSysemsProject1
Adding functionality that enables user to change color of the text and background to XV6 operating system.

Show and hide the table
It is necessary to support the display of a table with colors, and allow the user to navigate through that table and select an item. The table is entered by holding down the <ALT> key, and pressing the letter combination: c o l. It should also be possible to release the <ALT> key between individual letters, as long as nothing else is pressed between individual letters. The same key combination should hide the table and return the user to normal operation mode. While the table is displayed, nothing but the keys to move through the table should have any effect on the terminal. The <ALT> key should not be pressed while working with the table - only when showing and hiding it. After the table is removed from the screen, whatever was there before the table appeared should appear in its place.

Table position and layout
The table is located in the upper right corner of the screen, and should have two columns: one for choosing the font color, and the other for choosing the background color. Both columns have eight items - one for each base color supported in xv6: Black, Blue, Green, Aqua, Red, Purple, Yellow, White. The table should be displayed in bright white (0xf) on a black background (0x0). The table is displayed in these colors even if some other color combination is selected for the rest of the terminal. The current position within the table should be marked with reversed colors - black letters on a bright white background. In the following example, the user placed the cursor in the table on the item "Green" in the first column.

/---<FG>--- ---<BG>---\
|Black     |Black     |
|Blue      |Blue      |
|Green     |Green     |
|Aqua      |Aqua      |
|Red       |Red       |
|Purple    |Purple    |
|Yellow    |Yellow    |
|White     |White     |
\---------------------/

Work with a table
Moving through the table is done using the keys: w, a, s, and d, specifically:
w - movement up.
s - downward movement.
a - moving to the left (ie, changing the active column).
d - movement to the right (ie change of the active column).
The e and r keys are used to select an item in the table, specifically:
e - selects the currently selected color in its basic form.
r - chooses the currently selected color in its lighter form.
