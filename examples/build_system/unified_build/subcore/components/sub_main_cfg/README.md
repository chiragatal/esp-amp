This is a workaround to support Kconfig processing for subcore main component

Due to the fact that subcore project doesnot support renamed `main` component, if we simply add `main` component to `EXTRA_COMPONENT_DIRS` of maincore project, it will overwrite maincore `main` component. As as workaround, this component is created and added to `EXTRA_COMPONENT_DIRS` of maincore project to trigger sdkconfig generation for subcore `main` component.
