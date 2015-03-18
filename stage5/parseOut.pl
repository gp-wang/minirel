#!/s/std/bin/perl
use strict;
system("qutest 11 > out.txt 2> /dev/stdout");
system("cat out.txt");

print("\n\n### Output Analysis (This is printed by perl script) ###\n");
open(INFILE, "out.txt") || die "open";

my $fProblemsExist = 0;
my $nNumNotMatched = 0;
my $nNumDups = 0;
my %matched;
while(<INFILE>)
{
    if (/([0-9]+)\s+([0-9]+)/)
    {
	if ($matched{$1} != 0)
	{
	    print("Duplicate: $1\n");
	    $fProblemsExist = 1;
	    $nNumDups++;
	}
	$matched{$1} = 1;
    }
}
for (my $i = 0; $i < 1000; $i++)
{
    if ($matched{$i} != 1)
    {
	print("Not joined: $i\n");
	$fProblemsExist = 1;
	$nNumNotMatched++;
    }
}

close(INFILE);

if (! $fProblemsExist)
{
    print("\nNo problems -- Join is working!\n\n");
}
else
{
    print("\nJoin results incorrect.\n");
    print("Number of tuples not matched:      $nNumNotMatched\n");
    print("Number of duplicate output tuples: $nNumDups\n\n");
}
print("### End analysis ###\n");

