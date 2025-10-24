# Command

```sh
valgrind feral testdir tests
```

# Output

```sh
total: 65, passed: 65, failed: 0, time: 752 ms
==245646==
==245646== HEAP SUMMARY:
==245646==     in use at exit: 10,133 bytes in 6 blocks
==245646==   total heap usage: 39,772 allocs, 39,766 frees, 6,147,356 bytes allocated
==245646==
==245646== LEAK SUMMARY:
==245646==    definitely lost: 0 bytes in 0 blocks
==245646==    indirectly lost: 0 bytes in 0 blocks
==245646==      possibly lost: 0 bytes in 0 blocks
==245646==    still reachable: 10,133 bytes in 6 blocks
==245646==         suppressed: 0 bytes in 0 blocks
==245646== Rerun with --leak-check=full to see details of leaked memory
==245646==
==245646== For lists of detected and suppressed errors, rerun with: -s
==245646== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```