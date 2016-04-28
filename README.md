# oryol-imgui
'Dear Imgui' wrapper module for Oryol (https://github.com/ocornut/imgui)

Oryol sample: https://floooh.github.com/oryol-samples/asmjs/ImGuiDemo.html

### How to integrate into your Oryol project:

First, add a new import to the fips.yml file of your project:

```yaml
imports:
    oryol-imgui:
        git:https://github.com/floooh/oryol-imgui.git
```

Next, add a new dependency 'IMUI' to your app's CMakeLists.txt file:

```cmake
fips_begin_app(MyApp windowed)
    ...
    fips_deps(IMUI)
fips_end_app()
```

Run 'fips gen' to fetch the new dependencies and rebuild the build files:

```bash
> ./fips gen
...
```
