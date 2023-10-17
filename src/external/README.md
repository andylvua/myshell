## External programs directory

> **WARNING**
> 
> All external programs that are shipped with `msh` should be placed in the root of this directory
> as a subdirectory containing external program's files.
>
> Each external program should provide correct `CMakeLists.txt` file with defined `${PROJECT_NAME}` 
> in the root of its directory.

It will be automatically parsed by the build system and added to the build process, ensuring
that `msh` will recognize it as an external program.

> **NOTE**
> 
> Name of external program's parent directory is insignificant and is not used by the build system. 
> However, it is recommended to use the same name as the name of the external program to avoid confusion.