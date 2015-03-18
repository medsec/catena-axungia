Catena-axungia
================
This is an application that searches for the optimal Catena parameters under the
supplied limitations.

Reference implementation:
https://github.com/medsec/catena

Academic paper:
<a href="http://www.uni-weimar.de/fileadmin/user/fak/medien/professuren/Mediensicherheit/Research/Publications/catena-v3.0.pdf">catena-v3.0.pdf</a>

Build options
-------------
Catena-axungia depends on the Catena reference implementation and expects to
find it in `../catena`. This can be changed by appending
`CATENADIR=../somepath` to the invocation of make.

Parameters
----------
* The required parameters `--max_time` and  `--max_memory` define the upper
  bounds for time and memory. An invocation of Catena on the same machine with
  the resulting parameters will on average stay below these bounds. Please note
  that setting `--max_memory` to a higher value than the systems current free
  memory may result in a segmentation fault.
* `--min_mhard` sets the lower bound for memory hardness. Note that
  Catena-Dragonfly satisfies only 1-memory-hardness. For Catena-Butterfly, the
  memory hardness equals the lambda parameter. The default value for
  `--min_mhard` is 2 and we recommend a value of at least 2 to fend of certain
  types of attacks. If you want to increase the memory usage at all cost, you
  can set `--min_mhard` to 1.
* `--iterations` denotes the number of iterations used to determine the runtime
  of Catena under the resulting parameters. Higher values increase stability and
  reduce the influence of fluctuations. The default value is set to 3.
* `--full_hash` enables the `FULLHASH` option of Catena. When provided, Blake2b
  will be used for every hash-function call. Without this parameter, consecutive
  calls to the hash function use a reduced version of Blake2b, called Blake2b-1.

Dependencies
------------
* Catena-reference  (https://github.com/medsec/catena)
* clang             (http://clang.llvm.org/)
* make              (http://www.gnu.org/software/make/) 
