# Saleae Analyzer ISO/IEC7816
This plugins is created for the Saleae Logic software
It is used to decode ISO7816 protocol data from captured waveforms.

The libraries required to build this custom analyzer are stored in another git repository, located here:
[https://github.com/saleae/AnalyzerSDK](https://github.com/saleae/AnalyzerSDK)

## Windows
### Build
To build on Windows, open the visual studio project in the Visual Studio folder, and build. The Visual Studio solution has configurations for 32 bit and 64 bit builds. You will likely need to switch the configuration to 64 bit and build that in order to get the analyzer to load in the Windows software.

### Debug
To debug on Windows, please first review the section titled `Debugging an Analyzer with Visual Studio` in the included `doc/Analyzer SDK Setup.md` document.

To build on Linux or OSX, run the build_analyzer.py script. The compiled libraries can be found in the newly created debug and release folders.

	python build_analyzer.py

Unfortunately, debugging is limited on Windows to using an older copy of the Saleae Logic software that does not support the latest hardware devices. Details are included in the above document.


## Linux
### Build
    mkdir build && cd build
    cmake ../
    make
    cp Analyzers/libISO7816Analyzer.so path/to/Logic2/Custom/analyzers -f

### Debug
To debug on Linux:
Call cmake with debug options:

    cmake -DCMAKE_BUILD_TYPE=Debug ../

Run logic and identify renderer thread:

    ps ax | grep Logic.*--type=renderer | head -n1 | cut -d " " -f1

Verify the thread loaded analyzer:

    lsof -p <PID> | grep libISO7816Analyzer.so

Attach gdb to the thread:

    (gdb) attach <PID>

Debug the Analyzer:

    (gdb) break ISO7816Analyzer::WorkerThread
