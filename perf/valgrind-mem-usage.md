# Command

```sh
valgrind feral testdir tests
```

# Output

```sh
total: 64, passed: 64, failed: 0, time: 866 ms
==142592==
==142592== HEAP SUMMARY:
==142592==     in use at exit: 10,133 bytes in 6 blocks
==142592==   total heap usage: 39,316 allocs, 39,310 frees, 6,193,846 bytes allocated
==142592==
==142592== LEAK SUMMARY:
==142592==    definitely lost: 0 bytes in 0 blocks
==142592==    indirectly lost: 0 bytes in 0 blocks
==142592==      possibly lost: 0 bytes in 0 blocks
==142592==    still reachable: 10,133 bytes in 6 blocks
==142592==         suppressed: 0 bytes in 0 blocks
==142592== Rerun with --leak-check=full to see details of leaked memory
==142592==
==142592== For lists of detected and suppressed errors, rerun with: -s
==142592== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```