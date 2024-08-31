#!/usr/bin/perl -w
use strict;
my $argsfile;
if ($ARGV[0] eq "--args") {
  shift(@ARGV);
  $argsfile = shift(@ARGV);
}

my $log = shift;
if (!defined($log)) {
  die "hi stupid, did you tell me where your merged-stat file is?\n";
}

open FILE, "$log" or die "cannot open $log\n";
my $h = <FILE>;
chop $h;
my @oldheader = split(/ /,$h);
my @headers;

my $seedpos;
for (my $i = 0; $i <= $#oldheader; $i++) {
  if ($oldheader[$i]=~/param(\d+)\)seed/) {
    $seedpos = $1;
  }elsif (defined($seedpos) && ($oldheader[$i]=~/param(\d+)\)([\w\d]+)/)) {
    my $tmp = $1-1;
    push @headers, "param${tmp})$2";
  }else{
    push @headers, $oldheader[$i];
  }
}
if (!defined($seedpos)) {
  print STDERR "no seed, nothing to average!\n";
  exit(0);
}else{
  print STDERR "SEED position $seedpos\n";
}

my %totals;
my %totalnum;
my $newpline;
my $totalv;
my $totalv_num;

while (1) {
  my $pline = <FILE>;
  if (!defined($pline)) {
    last;
  }
  chop $pline;
  my $vline = <FILE>;
  if (!defined($vline)) {
    die print "$log: format wrong\n";
  }
  chop $vline;

  $newpline = "";
  my @params = split(/ /,$pline);
  for (my $i = 0; $i <= $#params; $i++) {
    if ($i != $seedpos) {
      $newpline .= "$params[$i] ";
    }
  }
  print STDERR ">>$newpline\n";
  my $totalv = $totals{$newpline};
  my $totalv_num = $totalnum{$newpline};
  if (defined($totalv)) {
    my @v = split(/ /,$vline);
    die "wrong wrong wrong\n" if ($#v != $#$totalv);
    for (my $i = 0; $i <= $#v; $i++) {
      $totalv->[$i] += $v[$i];
    }
    $totalnum{$newpline} = ($totalv_num+1);
  }else{
    my @v = split(/ /,$vline);
    $totals{$newpline} = \@v;
    $totalnum{$newpline} = 1;
  }
}

for (my $i = 0; $i <= $#headers; $i++) {
  print "$headers[$i] ";
}
print "\n";

foreach $newpline (keys %totals) {
  print "$newpline\n";
  $totalv_num = $totalnum{$newpline};
  $totalv = $totals{$newpline};
  for (my $j = 0; $j <= $#$totalv; $j++) {
    my $avg = $totalv->[$j]/$totalv_num;
    if ($avg =~/\./) {
      printf("%.3f ",$avg);
    }else{
      print "$avg ";
    }
  }
  print "\n";
}

print STDERR "averaging roughly $totalv_num different seeds\n";
