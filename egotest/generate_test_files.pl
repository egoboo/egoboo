#!/usr/bin/env perl

use strict;
use warnings;

use File::Spec;
use File::Path;

sub outputTestFileObjC ($$%);
sub outputTestFileCPP ($$%);
sub findTestCasesInFile ($);
sub cleanTestCaseName ($);
sub doWarn(@);

die "usage: $0 {objc|cpp} <file1> [file2...]" if @ARGV < 1;

mkdir "gen";

my $useObjC = $ARGV[0] =~ /^objc$/i;
shift; # remove first argument

my @testCasesList;

for my $testFile (@ARGV) {
    my %testCasesInFile = findTestCasesInFile($testFile);
    next unless %testCasesInFile;
    push @testCasesList, keys %testCasesInFile;
    my $genFile = "gen/$testFile";
    {
        my ($volume, $directory, $file) = File::Spec->splitpath($genFile);
        my $dirToCreate = File::Spec->catpath($volume, $directory, '');
        File::Path::make_path($dirToCreate);
    }
    if ($useObjC) {
        $genFile =~ s/\.cpp$/.mm/;
        outputTestFileObjC($testFile, $genFile, %testCasesInFile)
    } else {
        outputTestFileCPP($testFile, $genFile, %testCasesInFile);
    }
}

unless ($useObjC) {
    open my $out, '>', "gen/TestMain.cpp" or die "Cannot open gen/TestMain.cpp: $!";
    
    for my $testCase (@testCasesList) {
        my $testCaseClean = cleanTestCaseName($testCase);
        print $out "int TestCase_$testCaseClean();\n";
    }
    
    print $out "\n";
    print $out "#include \"EgoTest/EgoTest_Handwritten.cpp\"\n\n";
    print $out "std::map<std::string, std::function<int(void)>> EgoTest::getTestCases()\n";
    print $out "{\n";
    print $out "    std::map<std::string, std::function<int(void)>> ret;\n";
    
    for my $testCase (@testCasesList) {
        my $testCaseClean = cleanTestCaseName($testCase);
        printf $out "    ret.insert(std::make_pair(\"%s\", &TestCase_%s));\n", $testCase, $testCaseClean;
    }
    
    print $out "    return ret;\n";
    print $out "}\n";
    close $out;
}

my $hasOutputtedExtern = 0;

sub outputTestFileObjC ($$%) {
    my ($testFile, $outFile, %testCases) = @_;
    unless (%testCases) {
        doWarn "Found no test cases in $testFile";
        return;
    }
    open my $out, '>', $outFile or die "Cannot open $outFile: $!";
    print $out "#include \"$testFile\"\n";
    print $out "XCTestCase *EgoTest::currentTestCase;\n" unless $hasOutputtedExtern++;
    
    for my $testCase (keys %testCases) {
        my $testCaseClean = cleanTestCaseName($testCase);
        print $out "\@interface TC_$testCaseClean : XCTestCase\n";
        print $out "\@end\n\n";
        print $out "\@implementation TC_$testCaseClean\n";
        print $out "static $testCase _$testCaseClean;\n";
        print $out "- (void)setUp { ::EgoTest::currentTestCase = self; _$testCaseClean.setUp(); }\n";
        print $out "- (void)tearDown { ::EgoTest::currentTestCase = self; _$testCaseClean.tearDown(); }\n";
        print $out "+ (void)setUp { ::EgoTest::currentTestCase = self; _$testCaseClean.setUpClass(); }\n";
        print $out "+ (void)tearDown { ::EgoTest::currentTestCase = self; _$testCaseClean.tearDownClass(); }\n";
        my @tests = @{$testCases{$testCase}};
        for my $test (@tests) {
            print $out "- (void)test_$test { ::EgoTest::currentTestCase = self; _$testCaseClean.$test(); }\n";
        }
        print $out "\@end\n\n";
    }
    close $out;
}

sub outputTestFileCPP ($$%) {
    my ($testFile, $outFile, %testCases) = @_;
    unless (%testCases) {
        doWarn "Found no test cases in $testFile";
        return;
    }
    open my $out, '>', $outFile or die "Cannot open $outFile: $!";
    print $out "#include \"$testFile\"\n";
    
    for my $testCase (keys %testCases) {
        my $testCaseClean = cleanTestCaseName($testCase);
        print $out "int TestCase_$testCaseClean()\n";
        print $out "{\n";
        print $out "    int failures = 0;\n";
        print $out "    $testCase testCase;\n";
        print $out "    $testCase *testCasePtr = &testCase;\n";
        my @tests = @{$testCases{$testCase}};
        for my $test (@tests) {
            print $out "    failures += EgoTest::handleTest(\"$test\", [testCasePtr]() mutable {testCasePtr->$test();});\n";
        }
        print $out "    return failures;\n";
        print $out "}\n";
    }
    close $out;
}

sub cleanTestCaseName ($) {
    my $testCaseClean = shift;
    $testCaseClean =~ s/::/_/g;
    return $testCaseClean;
}

sub doWarn(@) {
    my $message = join '', @_;
    my (undef, $file, $line) = caller;
    my $prefix = $useObjC ? "warning: " : "\e[1;33m";
    my $suffix = $useObjC ? "" : "\e[0m";
    print STDERR "$prefix$message at $file line $line.$suffix\n";
}

sub findTestCasesInFile ($) {
    my $testFile = shift;
    my $testContents;
    if (open my $in, '<', $testFile) {
        local $/; # read the entire file
        $testContents = <$in>;
        close $in;
    } else {
        doWarn "Cannot open $testFile: $!, ignoring";
        return;
    }
    
    my @namespaces;
    my $braceCount = 0;
    my %testCases;
    my $currentTestCase;
    
    while ($testContents =~ /(namespace\s+(\w+)\s*{|EgoTest_TestCase\s*\(\s*(\w+)\s*\)\s*{|EgoTest_Test\s*\(\s*(\w+)\s*\)|{|})/g) {
        my $token = $1;
        my $name = $2 || $3 || $4 || undef;
        if ($token eq '{') {
            $braceCount++;
        } elsif ($token =~ /^namespace/) {
            $braceCount++;
            push @namespaces, [$name, $braceCount];
        } elsif ($token eq '}') {
            $braceCount--;
            @namespaces = grep { $_->[1] <= $braceCount } @namespaces;
            undef $currentTestCase if $currentTestCase && $currentTestCase->[1] > $braceCount;
        } elsif ($token =~ /^EgoTest_TestCase/) {
            doWarn "Cannot nest test cases, trying to define $name under ", $currentTestCase->[1] if $currentTestCase;
            $braceCount++;
            my $namespaces = join '::', map { $_->[0] } @namespaces;
            my $testCase = $namespaces ? $namespaces . "::" . $name : $name;
            $currentTestCase = [$testCase, $braceCount];
            $testCases{$testCase} = [];
        } elsif ($token =~ /^EgoTest_Test/) {
            doWarn "No test case for test $name" unless $currentTestCase;
            push @{$testCases{$currentTestCase->[0]}}, $name;
        } else {
            doWarn "Unimplemented token '$token'";
        }
    }
    my $uglyWorkaroundForXcode = <<EOF;
    }}
EOF
    
    doWarn "Unbalanced braces? $braceCount left" if $braceCount;
    doWarn "Extra namespaces: ", join ",", map { '['.$_->[0].','.$_->[1].']' } @namespaces if @namespaces;
    doWarn "Current test case: [", $currentTestCase->[0], ',', $currentTestCase->[1], ']' if $currentTestCase;
        
    return %testCases;
}