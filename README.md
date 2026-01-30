![AtomRunnerPro Header](https://raw.githubusercontent.com/atomlabor/Atom-Runner-Pro/main/Atom_Runner_Pro.jpg)
# Atom Runner Pro (beta)

Atom Runner Pro is a standalone running application built for the Pebble smartwatch ecosystem.

I created this project because I wanted a tracker that prioritised stability and readability above all else. Many existing apps were either too complex, abandoned, or struggled with the different screen geometries of the Pebble family. This app is designed to be lightweight, battery-efficient, and robust enough to handle a long run without crashing.

## Features

### Universal Compatibility
The application is not just ported; it is optimised for every hardware generation.
* **Chalk (Time Round):** Uses a specifically lifted layout to ensure no text is cut off by the circular display.
* **Diorite (Pebble 2):** High-contrast black and white optimisation to ensure clarity and prevent compiler warnings regarding colour variables.
* **Legacy Support:** Fully functional on Aplite (Pebble Classic/Steel) and Basalt (Time).

### The "Hybrid Coach"
This is the core analytical feature of the app. It adapts to the hardware available:
* **With Heart Rate Monitor:** On supported devices, it uses real-time sensor data to display your aerobic zone.
* **Without Heart Rate Monitor:** On older devices (or if the sensor fails), the app falls back to a "Virtual Estimate". It calculates your likely intensity zone based on your cadence (steps per minute). While not a medical measurement, it provides a reliable gauge of effort for training purposes.

### Background Persistence
The app includes a background worker process (I hope it works. Unfortunately, it's still a bit buggy.). This is critical for reliability. If you exit Atom Runner Pro to change music or check a notification, the session does not terminate. The app tracks your time and syncs your steps via the Health API in the background. When you reopen the app, your session resumes exactly where you left off.

### Data Storage
The watch stores your last 10 runs locally. You can review the date, distance, duration, and average heart rate (or steps) of previous sessions directly on the device.

## Installation and Build

To build this project, you will need the Pebble SDK (or the Rebble equivalent) installed on your machine.

1.  **Clone the repository**
    ```bash
    git clone [https://github.com/yourusername/atom-runner-pro.git](https://github.com/yourusername/atom-runner-pro.git)
    cd atom-runner-pro
    ```

2.  **Install dependencies**
    This project relies on `pebble-clay` for configuration.
    ```bash
    pebble npm install
    ```

3.  **Build the project**
    Run the build command. The script automatically handles the separate compilation of the main application and the background worker.
    ```bash
    pebble build
    ```

4.  **Install to watch/emulator**
    ```bash
    pebble install --emulator basalt
    # Or for a physical device:
    pebble install --phone <IP_ADDRESS>
    ```

**Note on Resources:** If you are building for the `emery` platform, ensure you have the required font file (`LECO_60.ttf`) placed in the `resources/fonts/` directory, otherwise the build will fail.

## How to Use

The interface is designed to be operated without looking at the screen for too long.

* **SELECT (Middle Button):**
    * *When stopped:* Starts the run.
    * *When running:* Stops and saves the run. You will see a summary screen. Press Select again to return to the main menu.

* **UP Button:**
    * *When running:* Toggles a high-visibility "Big Pace" mode. This hides secondary metrics and shows your current pace in a large font, ideal for quick checks during sprints.

* **DOWN Button:**
    * *When stopped:* Opens the History view. Scroll through your past 10 runs. Press Select on a specific run to view the "Coach Analysis", which gives feedback on your cadence and metabolic state.

## Project Structure

* `src/c/main.c`: Contains the UI logic, the hybrid coach algorithm, and the layout definitions for all platforms.
* `worker_src/c/worker.c`: A lightweight background process that keeps the application context alive during multi-tasking.
* `package.json`: Defines the target platforms and dependencies.

## License

This project is open source. Feel free to fork it, modify it, or use it to learn how to handle legacy Pebble hardware constraints.
All the best for the community, Please be kind to your neighbors and do not support fascists.

https://atomlabor.de 
