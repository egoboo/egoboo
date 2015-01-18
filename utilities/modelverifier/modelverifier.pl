#!/usr/bin/env perl

use strict;
use warnings;
use Fcntl 'SEEK_SET';
use Getopt::Long;

my $showWarnings = 1;
my $showList = 1;

GetOptions('warnings|w!' => \$showWarnings,
            'list|l!' => \$showList);

die <<EOF unless @ARGV;
$0: missing arguments.
$0 [-no-warnings] [-no-list] <path to Egoboo's data>
EOF

sub addObjects($);
sub checkMD2($);
sub contains($@);

my @actions = qw(DA DB DC DD UA UB UC UD TA TB TC TD CA CB CC CD SA SB SC SD BA BB
BC BD LA LB LC LD XA XB XC XD FA FB FC FD PA PB PC PD EA EB RA ZA ZB ZC ZD WA WB
WC WD JA JB JC HA HB HC HD KA KB KC KD MA MB MC MD ME MF MG MH MI MJ MK ML MM MN);

my $commandregex = qr/[LR][AGDC]|[AGDC][LR]*|I|S|F|P/;
my $extras = qr/^(?:$commandregex)*$/;
my $warnExtras = qr/\G(?:$commandregex|(.))/;

my $base = shift;
my $gobjDir = "$base/basicdat/globalobjects";
my $modDir = "$base/modules";

my @objects = ("$gobjDir/book.obj");
my %broken = ();
my %notbrk = ();

opendir my $gobjs, $gobjDir or die "cannot open global objects: $!";
addObjects("$gobjDir/$_") for grep { -d "$gobjDir/$_" && !/^\./ && !/\.obj$/ } readdir $gobjs;
closedir $gobjs;

opendir my $mods, $modDir or die "cannot open modules: $!";
addObjects("$modDir/$_/objects") for grep { -d "$modDir/$_" && /\.mod$/ } readdir $mods;
closedir $mods;


checkMD2($_) for @objects;

my %tmp;
if ($showWarnings) {
    $tmp{$_} = 1 for keys %broken, keys %notbrk;
} else {
    $tmp{$_} = 1 for keys %broken;
}

if ($showList) {
    for my $object (sort keys %tmp) {
        print "$object:";
        if (exists $broken{$object}) {
            my @tmp = @{$broken{$object}};
            printf " %d actionless frame%s: [%s]", scalar @tmp, @tmp == 1 ? "" : "s", join ', ', @tmp;
        }
        if (exists $notbrk{$object} && $showWarnings) {
            if (exists $notbrk{$object}{extras}) {
                my @tmp = @{$notbrk{$object}{extras}};
                printf " %d extra command%s: [%s]", scalar @tmp, @tmp == 1 ? "" : "s", join ', ', @tmp;
            }
            if (exists $notbrk{$object}{oddnumber}) {
                my @tmp = @{$notbrk{$object}{oddnumber}};
                printf " %d oddly sequenced frame%s: [%s]", scalar @tmp, @tmp == 1 ? "" : "s", join ', ', @tmp;
            }
            if (exists $notbrk{$object}{actions}) {
                my @tmp = @{$notbrk{$object}{actions}};
                printf " %d repeated action%s: [%s]", scalar @tmp, @tmp == 1 ? "" : "s", join ', ', @tmp;
            }
        }
        print "\n";
    }
}

my $errors = keys %broken;
my $warnings = keys %notbrk;
my $both = keys %tmp;
my $all = @objects;
printf "%d of %d model%s have issues. (%d error%s%s)\n", $both, $all, $all == 1 ? "" : "s", $errors, $errors == 1 ? "" : "s",
            $showWarnings ? (sprintf ", %d warning%s", $warnings, $warnings == 1 ? "" : "s") : "";

sub addObjects ($) {
    my $dir = shift;
    opendir my $dh, $dir or die "cannot open $dir: $!";
    push @objects, map { (-d "$dir/$_" && /\.obj$/ && -f "$dir/$_/tris.md2") ? "$dir/$_" : () } readdir $dh;
    closedir $dh;
}

sub checkMD2($) {
    my $object = shift;
    open my $fh, "<", "$object/tris.md2" or die "cannot open $object: $!";
    my $buffer;
    sysread $fh, $buffer, 8;
    my ($ident, $version) = unpack 'VV', $buffer;
    die "$object: invalid file" unless $ident == 0x32504449 && $version == 8;
    sysseek $fh, 16, SEEK_SET;
    sysread $fh, $buffer, 4;
    my $frameSize = unpack 'V', $buffer;
    sysseek $fh, 40, SEEK_SET;
    sysread $fh, $buffer, 4;
    my $numFrames = unpack 'V', $buffer;
    sysseek $fh, 56, SEEK_SET;
    sysread $fh, $buffer, 4;
    my $framePos = unpack 'V', $buffer;
    my $lastAction = "";
    my $lastActionFrame = -1;
    my @usedActions;
    for my $frame (0..($numFrames-1)) {
        my $pos = $framePos + $frameSize * $frame;
        sysseek $fh, $pos + 24, SEEK_SET;
        sysread $fh, $buffer, 16;
        my $frameName = unpack 'Z*', $buffer;
        if ($frameName =~ /^([A-Z]{2})(\d+)(.*)$/) {
            my $action = $1;
            my $actionFrameNum = $2;
            my $extra = $3;
            if (!contains($action, @actions)) {
                push @{$broken{$object}}, $frameName;
                next;
            }
            if ($extra !~ $extras) {
                my $buf = "";
                while ($extra =~ /$warnExtras/g) {
                    $buf .= $1 if defined $1;
                }
                push @{$notbrk{$object}{extras}}, "$frameName ($buf)";
                next;
            }
            if ($action ne $lastAction) {
                $lastAction = $action;
                $lastActionFrame = -1;
                if (contains($action, @usedActions)) {
                    push @{$notbrk{$object}{actions}}, "$frameName ($action)";
                } else {
                    push @usedActions, $action;
                }
            }
            if ($lastActionFrame + 1 != $actionFrameNum) {
                push @{$notbrk{$object}{oddnumber}}, "$frameName (wanted " . ($lastActionFrame + 1) . ")";
            }
            $lastActionFrame = $actionFrameNum;
        } else {
            push @{$broken{$object}}, $frameName;
            next;
        }
    }
    close $fh;
}

sub contains ($@) {
    my $txt = shift;
    for (@_) {
        return 1 if $txt eq $_;
    }
    return 0;
}
