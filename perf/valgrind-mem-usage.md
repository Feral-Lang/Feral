# Command

```sh
valgrind feral testdir tests
```

# Output

```sh
total: 64, passed: 64, failed: 0, time: 872 ms
==44103== 
==44103== HEAP SUMMARY:
==44103==     in use at exit: 10,133 bytes in 6 blocks
==44103==   total heap usage: 39,312 allocs, 39,306 frees, 6,193,686 bytes allocated
==44103== 
==44103== LEAK SUMMARY:
==44103==    definitely lost: 0 bytes in 0 blocks
==44103==    indirectly lost: 0 bytes in 0 blocks
==44103==      possibly lost: 0 bytes in 0 blocks
==44103==    still reachable: 10,133 bytes in 6 blocks
==44103==         suppressed: 0 bytes in 0 blocks
==44103== Rerun with --leak-check=full to see details of leaked memory
==44103== 
==44103== For lists of detected and suppressed errors, rerun with: -s
==44103== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```