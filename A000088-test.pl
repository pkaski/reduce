
use List::Util qw(shuffle);

$nv = shift;
$ne = $nv*($nv-1)/2;
$tv = $ne;
$eq = $tv + 2;
$n = $eq + $nv;
$m = $nv*($nv+1);
print "p edge $n $m\n";
for($i = 0; $i < $nv; $i++) {
    for($j = 0; $j <= $nv; $j++) {
	if($i == $j) {
	    print "e ".($eq + $i + 1)." ".($tv + 1)."\n";
	} else {
	    if($j == $nv) {
		print "e ".($eq + $i + 1)." ".($tv + 2)."\n";
	    } else {
		if($i < $j) {
		    $u = $i;
		    $v = $j;
		} else {
		    $v = $i;
		    $u = $j;
		}
		print "e ".($eq + $i + 1)." ".($u*$nv - $u*($u+1)/2 + $v - $u)."\n";
	    }
	}
    }
}
for($i = 1; $i <= $ne; $i++) {
    print "c $i 1\n";
}
for($i = 1; $i <= 2; $i++) {
    print "c ".($tv+$i)." ".($i+1)."\n";
}
for($i = 1; $i <= $nv; $i++) {
    print "c ".($eq+$i)." 4\n";
}
print "p variable $ne\n";
for($i = 1; $i <= $ne; $i++) {
    print "v $i u$i\n";
}
print "p value 2\n";
print "r ".($tv+1)." false\n";
print "r ".($tv+2)." true\n";
print "p prefix $ne 0 0\n";
@data = (1..$ne);
@cards = shuffle @data;
for($i = 0; $i < $ne; $i++) {
    print "f $cards[$i]\n";
}
