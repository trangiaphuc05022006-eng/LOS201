# Evidence Review: Boot Log vs. Build Log

## Observed successful guest startup

The checked-in [`logs/boot_log.txt`](../logs/boot_log.txt) records:

```text
Linux version 5.15.0
[ LAB-02 ] Loading lab2_driver module...
lab2_driver: /proc/lab2_info created
lab2_driver: loaded, major=240
[ LAB-02 ] lab2_driver loaded OK
```

Later in the same log, `lsmod` lists `lab2_driver`, `/dev/lab2` is a character device with major `240` and minor `0`, and `/proc/lab2_info` prints initial values (`Data length: 0 bytes`, `Open count: 0`). These observations support the statement that the module loaded in **that captured QEMU session**.

## Earlier failed build record

The separately tracked [`driver/build.log`](../driver/build.log) shows an invocation of the external-module build against Linux 5.15 and ends with assembler errors such as:

```text
Error: selected processor does not support `isb' in ARM mode
```

That log records **a failed build attempt**. It should not be used as evidence of successful compilation. The presence of a later boot log with a loaded module suggests that a working module was available for that guest session, but the repository does not establish a single unbroken command trace connecting the two logs.

## Evidence discipline

When preparing a report, keep build transcript, generated module checksum, kernel command line, module-load output, and test-script output together. Distinguish previously captured observations from newly proposed validation checks. This prevents accidentally presenting a design expectation as an experimentally observed fact.

## Source references

[`boot_log.txt`](../logs/boot_log.txt) and [`build.log`](../driver/build.log).
