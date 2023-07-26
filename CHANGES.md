# Changes and What's New in chkconfig

#### 0.9.7 (2023-07-25)

    * Addressed an issue in which empty default or state backing store
      directories was incorrectly treated as an error by making it a
      non-error case.

#### 0.9.6 (2023-07-21)

    * Addressed an issue in which --disable-docs did not disable
      building manual reference pages by adding a parallel option,
      --disable-man, expressly addressing the man page use case
      independently of Doxygen-based documentation.

#### 0.9.5 (2023-07-21)

    * Addressed formatting issues with several options in the manual
      reference page.

    * Added new sorting and comparison functions to the library:
      `chkconfig_flag_state_tuple_flag_compare_function` and
      `chkconfig_flag_state_tuple_state_compare_function`.

    * Added support for the flag/state origin concept and property in
      the library along with the following new interfaces:
      `chkconfig_state_get_with_origin` and
      `chkconfig_origin_get_origin_string`.

    * Added support for the *-o* / *--origin* command line option in
      the `chkconfig` command line interface utility.

#### 0.9.0 (2023-07-21)

    * Renamed `chkconfig_state_get_all` to `chkconfig_state_copy_all`
      to better reflect ownership semantics of allocated sources
      associated with the call.

    * Fully implemented support for the asserted
      `CHKCONFIG_OPTION_USE_DEFAULT_DIRECTORY` option for the
      `chkconfig_state_copy_all` and ``chkconfig_state_get_count`
      interfaces.

#### 0.7.1 (2023-07-20)

    * Addressed a number of issues in which some package files were being
      elided from package distribution archives.

#### 0.7.0 (2023-07-20)

    * First public release to Github.

