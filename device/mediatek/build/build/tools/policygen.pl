#!/usr/bin/perl
use Getopt::Long;
use XML::DOM;
use List::Util qw(first);
use List::MoreUtils qw(none any all uniq);
# TODO: enable warnings and strict to fix tons of warning...
#use warnings;
#use strict;
my $relPackageName= "";
my $company= "";
my $project= "";
my $baseProject = "";

&GetOptions(
    "rel=s" => \$relPackageName,
    "c=s" => \$company,
    "p=s" => \$project,
    "base=s" => \$baseProject,
);

if($relPackageName =~ /^\s*$/){
    print "release package is empty $relPackageName\n";
    &usage();
}

my $dom_parser = new XML::DOM::Parser;
my @relPackageNames=split /,/,$relPackageName;
my @relPackages = map { "device/mediatek/build/config/common/releasepackage/" . $_ . ".xml"} @relPackageNames;
my $doc;
my $platform;
my $nodes;
my $n;
my %configList = ();
my @dirList_removal = ();
my @dirList_r = ();
my @dirList_u = ();
my @fileList_r = ();
my @fileList_u = ();
my @frameworkList_s = ();
my @frameworkList_b = ();
my @androidList_s = ();
my @androidList_b = ();
my @appList_s = ();
my @appList_b = ();
my @kernelList = ();
my %chipdepList;
my $key;
my $is = 1;

sub dirName {
    my $path = shift;

    if ($path =~ m#^(.*)/$#) {
        return $1;
    } else {
        return $path;
    }
}

# Exit if the any node is found in both element groups.
# Params:
#   group1      1st group of hashes
#   group1      2nd group of hashes
sub assertNoDupPolicies {
    my @g1 = @{$_[0]};
    my @g2 = @{$_[1]};
    my @union = ();
    my %union = ();
    my @isect = ();
    my %isect = ();

    foreach my $e (@g1, @g2) {
        $union{$e}++ && $isect{$e}++;
    }

    @isect = keys %isect;
    if ($#isect >= 0) {
        print STDERR "[ERROR] Duplicated entry found:\n";
        for (my $i = 0; $i <= $#isect; $i++) {
            print STDERR "  * $isect[$i]\n";
        }

        exit 1
    }
}

foreach my $xmlfile (@relPackages) {
    $doc = $dom_parser->parsefile($xmlfile);

    if ($is == 1) {
        $platform = "";
        my $node = $doc->getElementsByTagName("ReleasePackageName")->item(0)->getChildNodes()->item(0);
        $platform = $node->getNodeValue if (defined($node));
        $is = 0;
    }

    my $configlists = $doc->getElementsByTagName("ConfigList");
    my $nc = $configlists->getLength;
    if ($nc > 0) {
        $nodes = $doc->getElementsByTagName("ConfigList")->item(0)->getElementsByTagName("Config");
        $n = $nodes->getLength;
        for (my $i = 0; $i < $n; $i++) {
            my $node = $nodes->item ($i)->getElementsByTagName("File")->item(0);
            my $fnode = $node->getChildNodes()->item(0) if (defined($node));
            my $fvalue = $fnode->getNodeValue if (defined($fnode));
            $node = $nodes->item ($i)->getElementsByTagName("Old")->item(0);
            my $onode = $node->getChildNodes()->item(0) if (defined($node));
            my $ovalue = $onode->getNodeValue if (defined($onode));
            $node = $nodes->item ($i)->getElementsByTagName("New")->item(0);
            my $nnode = $node->getChildNodes()->item(0) if (defined($node));
            my $nvalue = $nnode->getNodeValue if (defined($nnode));
            next if ($fvalue eq "" or $ovalue eq "");
            $fvalue = dirName("$fvalue");

            $configList{$fvalue}{$ovalue} = $nvalue;
        }
    }

    $nodes = $doc->getElementsByTagName("DirList")->item(0)->getElementsByTagName("ReleaseDirList")->item(0)->getElementsByTagName("Dir");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = "";
        $value = $node->getNodeValue if (defined($node));
        push(@dirList_r,$value) if ($value) && (none {$_ eq $value } @dirList_r);
        @dirList_u= grep (!/^$value$/,@dirList_u) if ((any {$_ eq $value} @dirList_u));
        @dirList_removal = grep (!/^$value$/,@dirList_removal) if ((any {$_ eq $value} @dirList_removal));
    }
    $nodes = $doc->getElementsByTagName("DirList")->item(0)->getElementsByTagName("UnReleaseDirList")->item(0)->getElementsByTagName("Dir");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = "";
        $value = $node->getNodeValue if (defined($node));
        push(@dirList_u,$value) if ($value) && (none {$_ eq $value } (@dirList_r,@dirList_u));
        @dirList_removal = grep (!/^$value$/,@dirList_removal) if ((any {$_ eq $value} @dirList_removal));
    }

    $nodes = $doc->getElementsByTagName("DirList")->item(0)->getElementsByTagName("RemovalDirList")->item(0)->getElementsByTagName("Dir");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        push(@dirList_removal,$value) if ($value) && (none {$_ eq $value } (@dirList_r,@dirList_removal,@dirList_u));
    }

    $nodes = $doc->getElementsByTagName("FileList")->item(0)->getElementsByTagName("ReleaseFileList")->item(0)->getElementsByTagName("File");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = "";
        $value = $node->getNodeValue  if (defined($node));
        push(@fileList_r,$value) if ($value) && (none {$_ eq $value } @fileList_r);
        @fileList_u = grep (!/^$value$/,@fileList_u) if (any {$_ eq $value} @fileList_u);
    }

    $nodes = $doc->getElementsByTagName("FileList")->item(0)->getElementsByTagName("UnReleaseFileList")->item(0)->getElementsByTagName("File");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        push(@fileList_u,$value) if ($value) && (none {$_ eq $value } @fileList_u);
    }

    $nodes = $doc->getElementsByTagName("FrameworkRelease")->item(0)->getElementsByTagName("SourceList")->item(0)->getElementsByTagName("Source");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = "";
        $value = $node->getNodeValue if (defined($node));
        push(@frameworkList_s,$value) if ($value) && (none {$_ eq $value } @frameworkList_s);
        @frameworkList_b  = grep (!/^$value$/,@frameworkList_b) if (any {$_ eq $value} @frameworkList_b);
    }

    $nodes = $doc->getElementsByTagName("FrameworkRelease")->item(0)->getElementsByTagName("BINList")->item(0)->getElementsByTagName("Binary");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        my $att = $nodes->item ($i)->getAttributes()->item(0);
        my $att_value = "";
        $att_value = $att->getNodeValue if (defined($att));
        $chipdepList{$value} = $att_value if ($att_value eq "yes");

        push(@frameworkList_b,$value) if ($value) && (none {$_ eq $value } @frameworkList_b);
    }


    $nodes = $doc->getElementsByTagName("AndroidRelease")->item(0)->getElementsByTagName("SourceList")->item(0)->getElementsByTagName("Source");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = "";
        $value = $node->getNodeValue if (defined($node));
        $value =~ s#\{COMPANY\}#$company# if ($company);
        $value =~ s#\{PROJECT\}#$project# if ($project);
        $value =~ s#\{BASE_PROJECT\}#$baseProject# if($baseProject);
        push(@androidList_s,$value) if ($value) && (none {$_ eq $value } @androidList_s);
        @androidList_b =  grep (!/^$value$/,@androidList_b ) if (any {$_ eq $value} @androidList_b);
    }

    $nodes = $doc->getElementsByTagName("AndroidRelease")->item(0)->getElementsByTagName("BINList")->item(0)->getElementsByTagName("Binary");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        my $att = $nodes->item ($i)->getAttributes()->item(0);
        my $att_value = "";
        $att_value = $att->getNodeValue if (defined($att));
        $chipdepList{$value} = $att_value if ($att_value eq "yes");
        push(@androidList_b,$value) if ($value) && (none {$_ eq $value } @androidList_b);
    }

    $nodes = $doc->getElementsByTagName("APPRelease")->item(0)->getElementsByTagName("SourceList")->item(0)->getElementsByTagName("Source");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = "";
        $value = $node->getNodeValue if (defined($node));
        push(@appList_s,$value) if ($value) && (none {$_ eq $value } @appList_s);
        @appList_b =  grep (!/^$value$/,@appList_b ) if (any {$_ eq $value } @appList_b);
    }

    $nodes = $doc->getElementsByTagName("APPRelease")->item(0)->getElementsByTagName("BINList")->item(0)->getElementsByTagName("Binary");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = "";
        $value = $node->getNodeValue if (defined($node));
        my $att = $nodes->item ($i)->getAttributes()->item(0);
        my $att_value = "";
        $att_value = $att->getNodeValue if (defined($att));
        $chipdepList{$value} = $att_value if ($att_value eq "yes");
        push(@appList_b,$value) if ($value)&& (none {$_ eq $value } @appList_b);
    }
    $nodes = $doc->getElementsByTagName("KernelRelease")->item(0)->getElementsByTagName("SourceList")->item(0)->getElementsByTagName("Source");
    $n = $nodes->getLength;
    for (my $i = 0; $i < $n; $i++) {
        my $node = $nodes->item ($i)->getChildNodes()->item(0);
        my $value = $node->getNodeValue if (defined($node));
        push(@kernelList,$value) if ($value) && (none {$_ eq $value } @kernelList);
    }

    $doc->dispose;
}

# Detect duplications
my @srcs = grep {$_ ne ""} uniq(@androidList_s, @frameworkList_s, @appList_s, @dirList_r, @kernelList);
my @bins = grep {$_ ne ""} uniq(@androidList_b, @frameworkList_b, @appList_b);
assertNoDupPolicies(\@srcs, \@bins);

my @temp = sort(@dirList_r, @frameworkList_s,@fileList_r,@androidList_s,@appList_s);
my $tmp = $temp[0];
my @duplicates = ();
for (my $i = 1; $i <= $#temp; $i++) {
    if ($temp[$i] =~ m#^$tmp/#) {
        push(@duplicates, $temp[$i]);
    } else {
        $tmp = $temp[$i];
    }
}

my @temp_r = sort(@dirList_removal, @dirList_u, @fileList_u);
my $tmp_r = $temp_r[0];
my @duplicates_r = ();
for (my $i = 1; $i <= $#temp_r; $i++) {
    if ($temp_r[$i] =~ m#^$tmp_r/#) {
        push(@duplicates_r, $temp_r[$i]);
    } else {
        $tmp_r = $temp_r[$i];
    }
}

print "<ReleasePackage>\n";

 print "\t<ReleasePackageName>";
 print "$platform";
 print "</ReleasePackageName>\n";

 print "\t<ConfigList>\n";
   foreach $key (sort keys %configList) {
     foreach my $oldConfig (sort keys %{$configList{$key}}){
       print "\t\t<Config>\n";
         print "\t\t\t<File>$key</File>\n";
         print "\t\t\t<Old>$oldConfig</Old>\n";
         print "\t\t\t<New>$configList{$key}{$oldConfig}</New>\n";
       print "\t\t</Config>\n";
     }
   }
 print "\t</ConfigList>\n";

 print "\t<DirList>\n";
  print "\t\t<RemovalDirList>\n";
   foreach $key (@dirList_removal) {
       if(none {$_ eq $key} @duplicates_r) {
           print "\t\t\t<Dir>$key</Dir>\n";
       }
   }
  print "\t\t</RemovalDirList>\n";
  print "\t\t<ReleaseDirList>\n";
   foreach $key (sort @dirList_r) {
       if(none {$_ eq $key} @duplicates) {
           print "\t\t\t<Dir>$key</Dir>\n";
       }
   }
  print "\t\t</ReleaseDirList>\n";
  print "\t\t<UnReleaseDirList>\n";
   foreach $key (sort @dirList_u) {
       if(none {$_ eq $key} @duplicates_r) {
           print "\t\t\t<Dir>$key</Dir>\n";
       }
   }
  print "\t\t</UnReleaseDirList>\n";
 print "\t</DirList>\n";

 print "\t<FileList>\n";
  print "\t\t<ReleaseFileList>\n";
   foreach $key (sort @fileList_r) {
       if(none {$_ eq $key} @duplicates) {
           print "\t\t\t<File>$key</File>\n";
       }
   }
  print "\t\t</ReleaseFileList>\n";
  print "\t\t<UnReleaseFileList>\n";
   foreach $key (sort @fileList_u) {
       if(none {$_ eq $key} @duplicates_r) {
           print "\t\t\t<File>$key</File>\n";
       }
   }
  print "\t\t</UnReleaseFileList>\n";
 print "\t</FileList>\n";

 print "\t<FrameworkRelease>\n";
  print "\t\t<SourceList>\n";
   my $result = "_A_";
   foreach $key (sort @frameworkList_s) {
       if(none {$_ eq $key} @duplicates) {
           print "\t\t\t<Source>$key</Source>\n";
           $result = $key;
       }
   }
  print "\t\t</SourceList>\n";
  print "\t\t<BINList>\n";
   $result = "_A_";
   foreach $key (sort @frameworkList_b) {
       if ( "$key/" !~ m#^$result/#) {
           if (exists $chipdepList{$key}){
               print "\t\t\t<Binary platform = \"yes\">$key</Binary>\n";}
           else {
               print "\t\t\t<Binary>$key</Binary>\n";}
           $result = $key;
       }
   }
  print "\t\t</BINList>\n";
  print "\t\t<PartialSourceList>\n";
 #  foreach $key (sort keys %frameworkList) {
 #      if (($frameworkList{$key} ne "Source") && ($frameworkList{$key} ne "Binary")) {
 #          my @data = split(':', $frameworkList{$key});
 #          my $base = (split("#", $key))[0];
 #          my $module = (split("#", $key))[1];
  my $module = "";
  my $base = "";
  print "\t\t\t<PartialSource module=\"${module}\" base=\"${base}\">\n";
 #           $result = "_A_";
 #           foreach $binary (@data) {
 #               if ("$binary/" !~ m#^$result/#) {
 #                   print "\t\t\t\t<Binary>$binary</Binary>\n";
 #                   $result = $binary;
 #               }
 #           }
  print "\t\t\t</PartialSource>\n";
 #      }
 #  }
  print "\t\t</PartialSourceList>\n";
 print "\t</FrameworkRelease>\n";

 print "\t<AndroidRelease>\n";
  print "\t\t<SourceList>\n";
   foreach $key (sort @androidList_s) {
       if(none {$_ eq $key} @duplicates) {
           print "\t\t\t<Source>$key</Source>\n";
       }
   }
  print "\t\t</SourceList>\n";
  print "\t\t<BINList>\n";
   foreach $key (sort @androidList_b) {
       if ( "$key/" !~ m#^$result/#) {
           if (exists $chipdepList{$key}){
               print "\t\t\t<Binary platform = \"yes\">$key</Binary>\n";}
           else {
               print "\t\t\t<Binary>$key</Binary>\n";}

       }
   }
  print "\t\t</BINList>\n";
 print "\t</AndroidRelease>\n";

 print "\t<APPRelease>\n";
  print "\t\t<SourceList>\n";
   foreach $key (sort @appList_s) {
       if(none {$_ eq $key} @duplicates) {
           print "\t\t\t<Source>$key</Source>\n";
       }
   }
  print "\t\t</SourceList>\n";
  print "\t\t<BINList>\n";
   foreach $key (sort @appList_b) {
       if ( "$key/" !~ m#^$result/#) {
           if (exists $chipdepList{$key}){
               print "\t\t\t<Binary platform = \"yes\">$key</Binary>\n";}
           else {
               print "\t\t\t<Binary>$key</Binary>\n";}
       }
   }
  print "\t\t</BINList>\n";
 print "\t</APPRelease>\n";

 print "\t<KernelRelease>\n";
  print "\t\t<SourceList>\n";
   foreach $key (sort keys @kernelList) {
       if(none {$_ eq $key} @duplicates) {
           print "\t\t\t<Source>$key</Source>\n";
       }
   }
  print "\t\t</SourceList>\n";
  print "\t\t<BINList>\n";
  print "\t\t</BINList>\n";
 print "\t</KernelRelease>\n";

print "</ReleasePackage>\n";
exit 0;

sub usage {
  warn << "__END_OF_USAGE";
Usage: perl policygen.pl [options]

Options:
  -rel      : release policy file.
  -p        : project to release.
  -c        : customer to release.

Example:
  perl policygen.pl -rel rel_customer_basic,rel_customer_platform_mt6752,rel_customer_modem -p k2v1 -c mediatek

__END_OF_USAGE

  exit 1;

}
