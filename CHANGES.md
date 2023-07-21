# Changes and What's New in chkconfig

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

