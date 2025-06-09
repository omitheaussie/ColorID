# Screen Color Highlighter (Windows Overlay App)

A Windows 11 desktop application that takes a snapshot of a selected screen and overlays it full-screen, making the screen appear frozen. It then tracks the mouse cursor color and highlights matching regions in the snapshot in real time.

---

## 🧩 Features

- 🔍 **Screen Snapshot & Overlay**  
  On launch, the app captures a still image of a selected screen (1 to 4 supported) and displays it in full-screen borderless mode on that screen.

- 🖱️ **Color Highlighting**  
  As the mouse moves over the overlay, the app highlights areas in the snapshot that closely match the color under the cursor. Highlight transparency varies with match closeness.

- 🎨 **Dynamic Overlay Transparency**  
  - Entire overlay is darkened with a 70% (change as needed) transparent gray by default.  
  - Color-matching regions become more transparent (more visible) based on how close they are to the hovered color.

- 🖥️ **Multi-Monitor Support**  
  Supports up to 4 screens. Select a target screen from a dropdown in a control window. Limitation: Currently the mouse color is only correctly picked up from selected screen. Also you need to change the screen selection in the drop box one time for this to be initiated.

- 🧼 **Minimal Control UI**  
  A small (300x300) always-on-top window shows:  
  - Text: `"Press Alt+Esc to exit"`  
  - Dropdown to select which screen to use  
  - Background color that matches the current cursor color

- ❌ **Exit via Alt+Esc**  
  Overlay remains full-screen until the user presses `Alt+Escape`, which allows the user to switch focus and close the app.

---

## 🚀 Getting Started

### Prerequisites

- Windows 11
- [Microsoft Visual Studio Professional 2019](https://visualstudio.microsoft.com/vs/)
- Desktop development with C++ workload
- GDI+ (included in Windows SDK)

---

## 🛠️ Building the Application

1. **Clone this repository:**

   ```bash
   git clone https://github.com/yourusername/screen-color-highlighter.git
   cd screen-color-highlighter

2. Open the solution in Visual Studio 2019.

3. **Build Configuration:**

    - Set configuration to Release x64
    - Ensure entry point is WinMain
    - Ensure gdiplus.lib is linked

4. **Build Solution:**

    - Press Ctrl+Shift+B or click Build > Build Solution
    - Run the compiled .exe file.
