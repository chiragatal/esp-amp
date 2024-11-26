Please don't rename this folder. Renamed `main` component is not support in subcore build system.

To support Kconfig processing for subcore main component, a dummy component `sub_main_cfg` can be created under `components` folder, and added to `EXTRA_COMPONENT_DIRS` of maincore project. More details can be found [here](../../../../docs/build_system.md).
