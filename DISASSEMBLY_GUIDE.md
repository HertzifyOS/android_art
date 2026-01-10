# Disassembling Android Platform Code (`oatdump` & `dex2oat`)

This guide covers workflows for inspecting compiled Android platform code (boot
image, system server, apps) using `oatdump` and analyzing compiler decisions
using `dex2oat`.

## 1. Tools Overview

*   **`oatdump`**: Dumps the content of `.oat` (ELF) and `.vdex` files. Use this
    to inspect code that **has already been compiled** on the build server or
    device.
*   **`dex2oat` (and `dex2oatd`)**: The on-device compiler. Use this locally to
    **simulate compilation** with debug flags (like `--dump-cfg`) to understand
    *why* and *how* code is being compiled (or not).

## 2. Inspecting Compiled Code with `oatdump`

### Locating Files

*   **Host (Build Output):**
    *   OAT files: `$OUT/system/framework/<arch>/boot.oat`
    *   VDEX files: `$OUT/system/framework/<arch>/boot.vdex`
    *   *Note*: `$OUT` (e.g., `out/target/product/raven`) is set after running
        the `lunch` command. `<arch>` (e.g., `arm64`) is a placeholder for your
        specific build architecture.
    *   *Note*: On newer Android versions (Android 11+), core libraries
        (java.lang, java.util) are in the ART APEX:
        `$OUT/apex/com.android.art/javalib/<arch>/boot.oat`.
*   **Device:**
    *   Path: `/system/framework/<arch>/boot.oat` (and `.vdex`)
    *   *Note: `<arch>` is a placeholder for the device architecture (e.g.,
        `arm64`).*
    *   Note: On newer Android versions, some artifacts might be in
        `/data/dalvik-cache` if updated or compiled on-device.

### Workflow A: Running on Host

Run `oatdump` against the build artifacts. This is faster and allows using
standard linux tools (`grep`, `less`) easily.

```bash
# Setup environment
source build/envsetup.sh
lunch <your_target>  # e.g., lunch raven-userdebug

# Basic header dump (check compiler filter, etc.)
oatdump --oat-file=$OUT/system/framework/arm64/boot.oat --header-only

# Disassemble a specific class
# Note: "android.os.Looper" is in framework.jar (boot extension).
# For java.util.* (core-oj), check the ART APEX path.
oatdump --oat-file=$OUT/system/framework/arm64/boot.oat \
        --class-filter=android.os.Looper \
        --disassemble-code
```

**Tip:** If you see "NO CODE", the method might not have been compiled
(interpreted only) or compiled in a different image (e.g., the app image vs boot
image).

### Workflow B: Running on Device

Useful if you don't have a matching local build or want to inspect the exact
state of a running device.

```bash
# Shell into device
# Note: 'adb root' requires a userdebug or eng build.
adb root && adb shell

# Dump to a file (recommended for large outputs)
oatdump --oat-file=/system/framework/arm64/boot.oat \
        --class-filter=android.os.Looper \
        --disassemble-code > /data/local/tmp/dump.txt

# Exit and pull
exit
adb pull /data/local/tmp/dump.txt
```

### Filtering Output

*   `--class-filter=<package.Class>`: Dumps only the specified class.
*   `--method-filter=<method_name>`: Dumps only methods with this name (e.g.,
    `get`).
*   `--no-disassemble`: Skips native code disassembly (useful for just checking
    OAT/Dex structure).
*   `--header-only`: Just the file header (checksums, key-value store,
    compiler-filter).

## 3. Analyzing Compiler Decisions with `dex2oat`

If `oatdump` shows "NO CODE" or suboptimal code, you can use `dex2oat` to
**recompile** the DEX file locally with verbose logging or CFG dumping.

### Workflow: Generating Control Flow Graphs (CFG)

The `--dump-cfg` flag generates a `.cfg` file that can be visualized using tools
like **c1visualizer** or **IR Hydra**. This shows the Intermediate
Representation (IR) at each compilation pass (inlining, constant folding,
register allocation, etc.).

1.  **Locate the Input DEX/JAR:**

    *   Example:
        `out/soong/.intermediates/libcore/core-oj/android_common/aligned/core-oj.jar`

2.  **Run `dex2oatd` (Debug Version):** Use `dex2oatd` (debug build) for better
    assertions and logging. Note that manually running `dex2oat` often requires
    specifying the boot class path if the code has dependencies.

    ```bash
    # Output file for the CFG
    CFG_OUTPUT=output.cfg

    $ANDROID_HOST_OUT/bin/dex2oatd \
      --dex-file=out/soong/.intermediates/libcore/core-oj/android_common/aligned/core-oj.jar \
      --oat-file=/dev/null \
      --boot-image=$OUT/system/framework/arm64/boot.art \
      --instruction-set=arm64 \
      --compiler-filter=speed \
      --dump-cfg=$CFG_OUTPUT \
      --verbose-methods=ArrayList.add,ArrayList.addAll
    ```

    *Note: If the compilation fails due to missing dependencies, you may need to
    provide `--boot-image` (e.g.,
    `--boot-image=$OUT/system/framework/arm64/boot.art`) or a full
    `--class-loader-context`.*

3.  **Analyze:**

    *   Open `output.cfg` in **c1visualizer** or **IR Hydra**.
    *   Look for passes like `Inliner` to see if a method was inlined or
        rejected (and why).

## 4. Common Recipes

### Check "Why wasn't this method inlined?"

1.  Verify with `oatdump` that it is indeed a call (`bl`/`blr`) and not inlined
    code.
2.  Use the `dex2oat` CFG workflow above.
3.  In the visualization tool (e.g., **c1visualizer**), find the `Inliner` pass.
4.  Look for the call site. The graph or side-panel usually logs failure reasons
    (e.g., "recursive", "too big", "cold", "always throws").

### Check "Is this method compiled hot?"

1.  Run `oatdump --header-only` to check the global `compiler-filter`.
    *   `verify`: No compilation (interpreter/JIT only).
    *   `speed-profile`: Profile-guided compilation.
    *   `speed`: AOT compiled.
2.  If `speed-profile`, dump the method.
    *   If `NO CODE`, it wasn't hot enough in the profile.
    *   If code exists, it was profiled as hot.

### Disassembling System Server

The System Server contains most of the core Android services (ActivityManager,
WindowManager, etc.).

```bash
# On device
adb shell oatdump --oat-file=/system/framework/oat/arm64/services.odex \
                  --boot-image=/system/framework/boot.art \
                  --class-filter=com.android.server.am.ActivityManagerService \
                  --disassemble-code
```

**Important:** When dumping `.odex` or app `.oat` files, you often must provide
`--boot-image` so `oatdump` can resolve dependencies (like base classes or
methods in the boot classpath) to disassemble the code correctly.

### Disassembling Application Code

For installed APKs:

1.  Find the base.odex/vdex:

    ```bash
    adb shell pm path com.example.app
    ```

    (Usually in `/data/app/.../oat/arm64/`).

    *Note*: The command output starts with `package:`. You must remove this
    prefix when using the path.

2.  Run `oatdump`:

    ```bash
    bash adb shell oatdump --oat-file=/data/app/.../base.odex --disassemble-code
    ```
