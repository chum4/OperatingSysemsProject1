# OperatingSystemsProject1

This project adds functionality to change the text and background colors in the XV6 operating system.

## Video Showcase
[Watch the demonstration here](https://user-images.githubusercontent.com/61984454/212170533-36a0fffb-1368-4c55-9327-e03d6f32f430.mp4)

## Table Display and Navigation
The project supports a color selection table that allows users to navigate and select items. The table can be displayed or hidden using the key combination: `c o l`. Users can press the keys in sequence, with the option to release the key between individual letters as long as no other key is pressed.

While the table is displayed, only navigation keys will be active, and other keys will have no effect on the terminal. Once the table is hidden, the original display will return.

### Table Position and Layout
The table is located in the upper right corner of the screen and contains two columns:
- **Font Color**
- **Background Color**

Each column features eight items representing the base colors supported in XV6:
- Black
- Blue
- Green
- Aqua
- Red
- Purple
- Yellow
- White

The table will be displayed in bright white (0xf) on a black background (0x0) regardless of the terminal's current color scheme. The current selection within the table will be highlighted using reversed colors: black text on a bright white background.

Example layout:
/------ ------ | Black | Black | | Blue | Blue | | Green | Green | | Aqua | Aqua | | Red | Red | | Purple| Purple| | Yellow| Yellow| | White | White | ---------------------/

## Navigation Controls
- **Movement within the Table**
  - `w` - Move up
  - `s` - Move down
  - `a` - Move left (change active column)
  - `d` - Move right (change active column)

- **Item Selection**
  - `e` - Select the currently highlighted color in its basic form.
  - `r` - Select the currently highlighted color in its lighter form.

This implementation enhances user interaction by providing a visual and intuitive method to customize the terminal appearance in XV6.

