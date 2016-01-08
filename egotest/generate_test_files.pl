#!/usr/bin/env perl

use 5.012; # Better Unicode support
use utf8; # This is a UTF-8-encoded file
use strict;
use warnings;

use File::Spec;
use File::Path ();

sub findTestCasesInFile ($);
sub cleanTestCaseName ($);

sub outputGeneratedObjCFile ($%);
sub outputGeneratedCPPFile ($%);

sub warning (@);
sub error (@);
sub debug (@);

# Current location of a file being processed, for warnings
# They are global variables so we can use dynamic scoping ('local $var;')
our $currentFile;
our $currentFileContents = "";

my $hasErrored = 0;
my $debug = 0;

# Set output of STDERR To UTF-8
binmode STDERR, ":encoding(UTF-8)";

die "usage: $0 {objc|cpp} <file1> [file2...]\n" if @ARGV < 1;

my $useObjC = $ARGV[0] =~ /^objc$/i;
shift; # remove first argument

my @testCasesList;

for my $testFile (@ARGV) {
    my %testCasesInFile = findTestCasesInFile($testFile);
    push @testCasesList, keys %testCasesInFile;
    
    my $genFile = "gen/$testFile";
    {
        my ($volume, $directory, $file) = File::Spec->splitpath($genFile);
        my $dirToCreate = File::Spec->catpath($volume, $directory, '');
        File::Path::make_path($dirToCreate);
    }
    
    $genFile =~ s/\.cpp$/.mm/ if $useObjC;
    
    my $out;
    
    unless (open $out, '>:encoding(UTF-8)', $genFile) {
        error "Couldn't open $genFile: $!";
        next;
    }
    
    print $out "#include \"$testFile\"\n";
    
    if ($useObjC) {
        outputGeneratedObjCFile($out, %testCasesInFile);
    } else {
        outputGeneratedCPPFile($out, %testCasesInFile);
    }
    
    close $out;
}

exit 1 if $hasErrored;

# Create gen/TestMain.cpp; this drives the handwritten backend
unless ($useObjC) {
    mkdir "gen";
    open my $out, '>:encoding(UTF-8)', "gen/TestMain.cpp" or die "Cannot open gen/TestMain.cpp: $!";
    
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

sub outputGeneratedObjCFile ($%) {
    my ($out, %testCases) = @_;
    
    if (%testCases) {
        print $out "\nstatic void setTestCase(XCTestCase *self) { ::EgoTest::currentTestCase = self; }\n";
        print $out "XCTestCase *EgoTest::currentTestCase;\n" unless $hasOutputtedExtern++;
    }
    
    my $testCaseNum = 0;
    
    for my $testCase (keys %testCases) {
        my $testCaseClean = cleanTestCaseName($testCase);
        my $testCaseVar = "testCase$testCaseNum"; $testCaseNum++;
        
        print $out "\n\@interface TC_$testCaseClean : XCTestCase\n";
        print $out "\@end\n\n";
        print $out "static $testCase $testCaseVar;\n\n";
        print $out "\@implementation TC_$testCaseClean\n\n";
        print $out "- (void)setUp { setTestCase(self); $testCaseVar.setUp(); }\n";
        print $out "- (void)tearDown { setTestCase(self); $testCaseVar.tearDown(); }\n";
        print $out "+ (void)setUp { setTestCase(self); $testCaseVar.setUpClass(); }\n";
        print $out "+ (void)tearDown { setTestCase(self); $testCaseVar.tearDownClass(); }\n\n";
        
        my @tests = @{$testCases{$testCase}};
        for my $test (@tests) {
            print $out "- (void)test_$test { setTestCase(self); $testCaseVar.$test(); }\n";
        }
        print $out "\n\@end\n";
    }
}

sub outputGeneratedCPPFile ($%) {
    my ($out, %testCases) = @_;
    
    for my $testCase (keys %testCases) {
        my $testCaseClean = cleanTestCaseName($testCase);
        print $out "\nint TestCase_$testCaseClean()\n";
        print $out "{\n";
        print $out "    int failures = 0;\n";
        print $out "    $testCase testCase;\n";
        #print $out "    $testCase *testCasePtr = &testCase;\n";
        my @tests = @{$testCases{$testCase}};
        for my $test (@tests) {
            # [testCasePtr]() mutable {testCasePtr->$test();}
            print $out "    failures += EgoTest::handleTest(\"$test\", std::bind(&${testCase}::$test, &testCase));\n";
        }
        print $out "    return failures;\n";
        print $out "}\n";
    }
}

sub cleanTestCaseName ($) {
    my $testCaseClean = shift;
    $testCaseClean =~ s/::/_/g;
    return $testCaseClean;
}

sub getCurrentLocation () {
    my $line = 1;
    my $column = 1;
    
    my $str = substr $currentFileContents, 0, pos $currentFileContents;
    my @lines = split /\n/, $str, -1;
    
    if (@lines) {
        $line = @lines;
        $column = 1 + length $lines[-1];
    }
    
    return ($line, $column);

}

sub doMessage ($$@) {
    my $type = shift;
    my $color = shift;
    my $message = join '', @_;
    
    my $file = $currentFile;
    my ($line, $column) = getCurrentLocation();
    
    # Xcode expects absolute file paths
    $file = File::Spec->rel2abs($file) if $file and $useObjC;
    
    my $location = $file ? "$file:$line:$column: " : "";
    my $prefix = $useObjC ? "$location$type: " : "\e[${color}m$location";
    my $suffix = $useObjC ? "" : "\e[0m";
    print STDERR "$prefix$message$suffix\n";
}

sub warning (@) {
    doMessage "warning", "1;33", @_;
}

sub error (@) {
    $hasErrored = 1;
    doMessage "error", "1;31",  @_;
}

sub debug (@) {
    return unless $debug;
    doMessage $useObjC ? "warning" : "debug", "1;37", @_;
}

sub getLocation($$) {
    my ($str, $pos) = @_;
    my $line = 1;
    my $column = 1;
    
    $str = substr $str, 0, $pos;
    my @lines = split /\n/, $str, -1;
    
    if (@lines) {
        $line = @lines;
        $column = 1 + length $lines[-1];
    }
    
    return [$line, $column];
}

sub findTestCasesInFile ($) {
    local $currentFile = shift;
    local $currentFileContents;
    if (open my $in, '<:encoding(UTF-8)', $currentFile) {
        local $/; # read the entire file
        $currentFileContents = <$in>;
        close $in;
    } else {
        warning "Cannot open: $!";
        return;
    }
    
    #if (
    
    my @namespaces;
    my $braceCount = 0;
    my %testCases;
    my $currentTestCase;
    
    # Usable whitespace:
    # space, horiz tab, vert tab, form feed, new line, backslash new line
    my $sp = qr/ |\t|\013|\f|\n|\\\n/;

    # Usable characters for identifiers: (ASCII only)
    my $id = qr/[A-Za-z_][A-Za-z_0-9]*/;
    
    # Usable characters for raw string delimiters:
    # printable ASCII minus '()\ '
    my $rawDelim = qr/[A-Za-z0-9_{}\[\]#<>%:;.?*+\-\/\^&|~!=,"']/;
    
    my $parser = qr<( # $1 will hold the token captured
        R"| # a raw string literal
        "| # a string literal
        '| # a character literal
        /\*| # a multi-line comment
        //| # a single-line comment
        namespace        $sp+         ($id)         $sp* {| # a new namespace with a scope, the identifier is in $2
        EgoTest_TestCase $sp* \( $sp* ($id) $sp* \) $sp* {| # a new testcase with a scope, the identifier is in $3
        EgoTest_Test     $sp* \( $sp* ($id) $sp* \) $sp* {| # a new test with a scope, the identifier is in $4
        {| # a new scope
        } #the end of a scope
    )>x; # x modifier ignores whitespace and comments inside the regex
    
    while ($currentFileContents =~ /$parser/gc) {
        my $token = $1;
        my $identifier = $2 || $3 || $4 || undef;
    
        $token =~ s/($sp|\().*$//s; # Remove everything after the actual token we care about
        
        debug sprintf "token: %s%s, namespace: %s, bracecount: %d",
            defined $token ? $token : "undef",
            defined $identifier ? " \"$identifier\"" : "",
            $currentTestCase ? $currentTestCase->[0] : (join '::', map { $_->[0] } @namespaces) || "::",
            $braceCount;
        
        if ($token eq '{') {
            $braceCount++;
        } elsif ($token eq 'namespace') {
            $braceCount++;
        
            push @namespaces, [$identifier, $braceCount];
        } elsif ($token eq '}') {
            if ($braceCount == 0) {
                error "Found erroneous closing brace";
                next;
            }
            $braceCount--;
        
            @namespaces = grep { $_->[1] <= $braceCount } @namespaces;
            undef $currentTestCase if $currentTestCase && $currentTestCase->[1] > $braceCount;
        } elsif ($token eq 'EgoTest_TestCase') {
            $braceCount++;
        
            if ($currentTestCase) {
                error "Cannot define test case '$identifier' under ", $currentTestCase->[0];
                return;
            }
        
            my $namespaces = join '::', map { $_->[0] } @namespaces;
            my $testCase = $namespaces ? $namespaces . "::" . $identifier : $identifier;
        
            $currentTestCase = [$testCase, $braceCount];
            $testCases{$testCase} = [];
        } elsif ($token eq 'EgoTest_Test') {
            $braceCount++;
        
            unless ($currentTestCase) {
                error "Cannot define test '$identifier' with no test case";
                next;
            }
        
            push @{$testCases{$currentTestCase->[0]}}, $identifier;
        } elsif ($token eq '//') {
            my $failed = 1;
        
            while ($currentFileContents =~ /(\\\n|\n)/gc) {
                if ($1 eq "\n") {
                    $failed = 0;
                    last;
                }
            }
            
            if ($failed) {
                # The only way we can fail is end-of-file,
                # and end-of-file is a valid terminator for //.
                # As we know it's the end-of-file, we can terminate the loop early
                last;
            }
        } elsif ($token eq '/*') {
            unless ($currentFileContents =~ m{\*/}gc) {
                error "Couldn't find terminating */";
                last;
            }
        } elsif ($token eq '"') {
            my $failed = 1;
            
            while ($currentFileContents =~ /(\\(\\\n)*.|"|\n)/sgc) { #"\\␤"" -> "\""
                if ($1 eq '"') {
                    $failed = 0;
                    last;
                }
                
                if ($1 eq "\n") {
                    last;
                }
            }
            
            if ($failed) {
                warning "Couldn't find a terminating \"";
            }
        } elsif ($token eq "'") {
            my $failed = 1;
            
            while ($currentFileContents =~ /(\\(\\\n)*.|')/gc) {
                if ($1 eq "'") {
                    $failed = 0;
                    last;
                }
            }
            
            if ($failed) {
                warning "Couldn't find a terminating '";
            }
        } elsif ($token eq 'R"') {
            # Max delimiter size is 16
            unless ($currentFileContents =~ /($rawDelim{0,16})\(/gc) {
                error "Cannot find a valid raw string delimiter";
                last;
            }
            
            my $delim = $1;
            my $quotedDelim = quotemeta $delim; # escape the delimiter for regexing
            
            unless ($currentFileContents =~ /\)$quotedDelim"/gc) {
                error "Cannot find a terminating ')$delim\"' for a raw string"
            }
        } else {
            error "Unimplemented token '$token' (this shouldn't happen)";
        }
        
        debug sprintf "end token: %s%s, namespace: %s, bracecount: %d",
            defined $token ? $token : "undef",
            defined $identifier ? " \"$identifier\"" : "",
            $currentTestCase ? $currentTestCase->[0] : (join '::', map { $_->[0] } @namespaces) || "::",
            $braceCount;
    }
    debug "Finished parsing here";
    
    # Set current position to the end of the file
    pos $currentFileContents = length $currentFileContents;
    
    if ($braceCount) {
        error "Missing $braceCount closing brace", $braceCount == 1 ? "" : "s";
    }
    
    if (@namespaces) {
        warning " - Extra namespaces: ", join(", ", map { "'" . $_->[0] . "' at brace level " . $_->[1] } @namespaces); 
    }
    
    if ($currentTestCase) {
        warning " - Current test case: '", $currentTestCase->[0], "' at brace level ", $currentTestCase->[1];
    }
    
    return %testCases;
}

# The rest of this is partially written code; we don't want to run this, so end the file here.
__END__

# Partial Unicode support for identifiers.
# Supported: UTF-8-encoded characters (clang only)
# Not Supported: UCNs (\u0000, \U00000000)

# A list of allowed characters for identifiers; due to the nature of \p{} in regexes,
# these two functions must be defined before the regular expression is compiled.
sub InIdentifierSet {
    return <<EOF;
30 39
41 5A
5F
61 7A
A8
AA
AD
AF
B2 B5
B7 BA
BC BE
C0 D6
D8 F6
F8 FF
0100 167F
1681 180D
180F 1FFF
200B 200D
202A 202E
203F 2040
2054
2060 206F
2070 218F
2460 24FF
2776 2793
2C00 2DFF
2E80 2FFF
3004 3007
3021 302F
3031 303F
3040 D7FF
F900 FD3D
FD40 FDCF
FDF0 FE44
FE47 FFFD
10000 1FFFD
20000 2FFFD
30000 3FFFD
40000 4FFFD
50000 5FFFD
60000 6FFFD
70000 7FFFD
80000 8FFFD
90000 9FFFD
A0000 AFFFD
B0000 BFFFD
C0000 CFFFD
D0000 DFFFD
E0000 EFFFD
EOF
}

# A list of removed characters that are not allowed to start identifiers.
sub InIdentifierInitialSet {
    return <<EOF;
+main::InIdentifierSet
-0300 036F
-1DC0 1DFF
-20D0 20FF
-FE20 FE2F
EOF
}

sub findTestCasesInFile {
    # UTF-8-encoded identifiers
    my $id = qr/\p{InIdentifierInitialSet}\p{InIdentifierSet}*/;
}
