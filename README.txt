
OVERVIEW
--------

This git repository contains C source code for 'reduce', an experimental 
software implementation of adaptive prefix-assignment symmetry reduction; cf.

  T. Junttila, M. Karppa, P. Kaski, J. Kohonen,
  "An adaptive prefix-assignment technique for symmetry reduction".

This experimental software is supplied to accompany the aforementioned 
manuscript.


LICENSE
-------

The source code is subject to the MIT License, see 'LICENSE.txt' for details.


BUILDING
--------

1)
This software uses the 'nauty' canonical labeling software for vertex-colored
graphs as a subroutine. Download the 'nauty' tar.gz-archive from

   http://users.cecs.anu.edu.au/~bdm/nauty/

to the subdirectory 'nauty/'. Unpack the archive in this subdirectory to
create the subdirectory 'nauty/nauty26r7/' (or later). 

2)
Configure and build 'nauty'. (Cf. 'nauty' documentation.)

3)
This software uses the 'gmplib', the GNU Multiple Precision Arithmetic Library
for large-integer arithmetic. Download the 'gmplib' tar.lz-archive from

   https://gmplib.org

to the subdirectory 'gmp/'. Unpack the archive in this subdirectory to
create the subdirectory 'gmp/gmp-6.1.2/' (or later).

4)
Configure and build 'gmplib'. (Cf. 'gmplib' documentation.)

5)
Edit 'Makefile' to configure the appropriate paths to 'nauty' (NAUTY_PATH) and 
'gmplib' (GMP_PATH) compiled object files. That is, the object/archive files 
described by NAUTY_OBJS and GMP_A should be available for the present build. 
(If this is not the case, reconfigure to match your configuration.)

6)
Run 'make' to build 'reduce'. 


TESTING
-------

It is strongly recommended to test the build using the accompanying perl-script
'A000088-test.pl'. This script uses 'reduce' to classify undirected graphs with
a specific number of vertices up to isomorphism. 

To test the software, run the commands

   perl A000088-test.pl 4 | ./reduce -ng
   perl A000088-test.pl 5 | ./reduce -ng
   perl A000088-test.pl 6 | ./reduce -ng
   perl A000088-test.pl 7 | ./reduce -ng

The output counts obtained should match the sequence A000088 in the Online
Encyclopedia of Integer Sequences (cf. https://oeis.org/A000088 ). Namely, 
the output counts from the four commands should be 11, 34, 156, and 1044, 
respectively.


USING 'reduce'
--------------

The command

  ./reduce -h

prints an usage display of the available command-line options. 

Usage without an explicit symmetry graph
----------------------------------------

By default, 'reduce' expects as input a CNF formula in DIMACS format 
followed by a description of the prefix. By default, the input is read
from the standard input. A simple example is as follows:

./reduce <<EOF
p cnf 6 3
1 2 0
1 3 5 0
2 4 6 0
p prefix 2 0 0
f 3
f 4
EOF

This input consists of a CNF instance with six variables and three clauses,
followed by a description of a prefix of length two, consisting of 
the variables 3 and 4 in this order. With this input, 'reduce' produces
the following output:

p cnf 9 10
1 2 0
1 3 5 0
2 4 6 0
-3 -7 0
-4 -7 0
-3 -8 0
4 -8 0
3 -9 0
4 -9 0
7 8 9 0

In particular, up to symmetries of the input CNF instance, there are exactly
three nonisomorphic assignments to the variables 3 and 4, namely -3 -4, -3 4, 
and 3 4. These a disjunction of these conjuncts has been encoded in the 
output of 'reduce'. 

Alternatively, the prefix maybe given as input from the command line via
the option '-p'. The following command is equivalent to the previous one:

./reduce -p 3 4 <<EOF
p cnf 6 3
1 2 0
1 3 5 0
2 4 6 0
EOF


Usage with CNF and an explicit symmetry graph
---------------------------------------------

In most cases one wants to use 'reduce' so that the input symmetries are
described explicitly by a graph given in the DIMACS format. Here is an
example that contains a system of clauses in CNF, followed by a graph
representation of the symmetry in the system, given in the DIMACS graph format:

./reduce -g <<EOF
p cnf 6 3
1 2 0
1 3 5 0
2 4 6 0
p edge 8 4
e 1 3
e 1 5
e 2 4
e 2 6
c 1 1
c 2 1
c 3 1
c 4 1
c 5 1
c 6 1
c 7 2
c 8 3
p variable 6
v 1 1
v 2 2
v 3 3
v 4 4
v 5 5
v 6 6
p value 2
r 7 false
r 8 true
p prefix 6 0 0
f 1
f 2
f 3
f 4
f 5
f 6
EOF

Here the input consists of the input CNF, followed the symmetry graph and
the prefix. Let us look at the description of the symmetry graph in parts.
First, we describe the vertices and edges of the graph:

p edge 8 4
e 1 3
e 1 5
e 2 4
e 2 6

That is, the graph has 8 vertices and 4 edges. Each edge is described by
a unique 'e'-line of the format 'e <i> <j>' for an edge joining the
vertex <i> to the vertex <j>. The vertices are numbered from 1 to N, where
N is the number of vertices in the graph. Next, we describe the colors
of the vertices using 'c'-lines:

c 1 1
c 2 1
c 3 1
c 4 1
c 5 1
c 6 1
c 7 2
c 8 3

The format of a 'c'-line is 'c <i> <k>' that indicates that the vertex <i>
has color <k>. The color <k> must be a positive integer. 

Finally, we define which vertices of the symmetry graph correspond to which
variables and which values of the CNF instance. This is done using 
'v'-lines and 'r'-lines as follows:

p variable 6
v 1 1
v 2 2
v 3 3
v 4 4
v 5 5
v 6 6
p value 2
r 7 false
r 8 true

These lines declare that there are six variable-vertices in the symmetry
graph, corresponding to the six variables in the CNF instance. Each 'v'-line
gives one such correspondence, with the format 'v <i> <q>' indicating that
the vertex <i> in the graph corresponds to the variable <q> in the CNF 
instance above. Furthermore, there are two value-vertices in the graph,
each declared with an 'r'-line, with the format 'r <i> <v>' indicating
that the vertex <i> in the graph corresponds to the value <v> in the CNF
instance. Both values 'false' and 'true' must be present. Finally, the
prefix is described:

p prefix 6 0 0
f 1
f 2
f 3
f 4
f 5
f 6

The prefix consists of the six variable-vertices in the symmetry graph,
namely the vertices 1,2,3,4,5,6 in this order. Observe in particular that
the prefix refers to the variable-vertices in the symmetry graph, *not* 
to the variables of the CNF instance. 


Usage with the symmetry graph only
----------------------------------

We can also instruct 'reduce' not to expect CNF input at all. (For example,
we saw such usage with 'TESTING' above.) Below is an example that defines
a prefix of variable-vertices at the command line:

./reduce -ngp 1 2 3 <<EOF
p edge 8 12
e 6 4
e 6 1
e 6 2
e 6 5
e 7 1
e 7 4
e 7 3
e 7 5
e 8 2
e 8 3
e 8 4
e 8 5
c 1 1
c 2 1
c 3 1
c 4 2
c 5 3
c 6 4
c 7 4
c 8 4
p variable 3
v 1 u1
v 2 u2
v 3 u3
p value 2
r 4 false
r 5 true
EOF

Observe that the CNF instance is now missing from the input and that the input starts with the symmetry graph. This is followed by a description of the variable-vertices and the value-vertices. The format for the 'v'-lines is now 'v <i> <t>' where <i> is a vertex of the symmetry graph and <t> is an arbitrary textual identifier of the variable. Similarly, the 'r'-lines are given as 'r <i> <t>' where <i> is a vertex and <t> is an arbitrary textual identifier of the value. 

