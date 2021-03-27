PrimerPooler
============

From http://ssb22.user.srcf.net/pooler/

(mirrored at http://ssb22.gitlab.io/pooler/ just in case)

This is a program I wrote for cancer researchers and others who want to use
Multiplex PCR to study
DNA samples, and wish to optimise
their combinations of primers to minimise the formation of dimers.

Primer Pooler can:

* Check through each proposed pool for combinations that are likely to form dimers,

* Automatically move prospective amplicons between proposed pools to reduce dimer formation,

* Automatically search the genome sequence to find which amplicons overlap, and place their corresponding primers in separate pools,

* Optionally keep pool sizes within a specified range,

* Handle thousands of primers without being slow (useful for high-throughput sequencing applications),

* Do all of the above with degenerate primers too.

If your CPU is modern enough to have them, Primer Pooler will take advantage of 64-bit registers and multiple cores. But it also runs on older equipment.

Please note that Primer Pooler **does not design primers by itself.**  You must choose your primers first, whether by using NCBI's [Primer BLAST](https://www.ncbi.nlm.nih.gov/tools/primer-blast/index.cgi) or any other method of your choice.  Once you have your primers, Primer&nbsp;Pooler can partition them into pools.


Compiling from source
---------------------

You will need:

* a C compiler (GCC or Clang) and basic Unix tools if you want to compile for GNU/Linux, Mac, BSD, etc,

* MingW compiler(s) if you want to cross-compile for Windows.

Type `make` or `make win-crosscompile`

Usage
-----

The easiest way to run Primer Pooler for first-time users is to run it interactively. To do this, simply launch the program file (`pooler` or `pooler64`) and it should ask you a series of questions to take you through what you want to do.

Questions asked by Primer Pooler when running interactively:

Would you like to run interactively? (y/n):
: You should answer _y_ to this question, otherwise Primer Pooler will merely display the command-line help (see below) and exit.

Please enter the name of the primers file to read.
: As the program further explains, it is expecting a text file in multiple-sequence FASTA format, such as:

    >toySet1-F
    AGCTGCTGCTGCGATCT
    >toySet1-R
    GGCTGAGCGCTCAGTTT
    >toySet2-F
    ACGGCTTGACACCGTTCGACTG
    >toySet2-R
    CAGACGTTCAG

(this example does not represent real primers). Degenerate bases are allowed using the normal letters, and both upper and lower case is allowed. Names of amplicons’ primers should end with F or R (for Forward and Reverse), and otherwise match.

Optionally include tags to apply to all primers (also called _tailed primers_ or barcoding) using `>tagF` and `>tagR` (tags can also be changed part-way through the file). If you also have Taq probes or other primers that don’t themselves make amplicons, you can include these ending with other letters, e.g. `>toySet1-P`—any set of names differing in only the last letter will be kept in the same pool, but you must use F for forward and R or B for reverse (backward) if you also want to check primer-pairs for overlaps in the genome.

You can also manually “fix” a primer-set to a predetermined pool number by using a primer name prefix: `>@2:myPrimer-F` fixes `myPrimer-F` to pool 2 (in which case Primer Pooler will allocate other primer-sets around these limitations); this can be useful when you don’t have a whole-genome file for overlap detection.

Do you want to use deltaG? (y/n): 
: As the program explains, it will need to be told the temperature and concentration settings if you want it to use deltaG. Alternatively you can use the faster and simpler “score” method, but this is less accurate.

* If you opt to use Score when your primers and/or tags are very long, you will be asked if you are really sure you don’t want to use deltaG instead.

* If you opt for deltaG, the following questions will be asked:

Temperature:
: Enter a number (decimal fractions are allowed). You can enter it in Celsius, Kelvin, Fahrenheit or Rankine. Do not enter the suffix C or K or F or R—Primer Pooler will determine for itself which unit was meant, and ask you to confirm. (Recent versions of Primer Pooler offer 5 additional obscure temperature scales if you decline all of the more probable ones.)

Magnesium concentration in mM (0 for no correction):
: Enter your concentration of magnesium in nanomoles per cubic metre (decimal fractions are allowed). Enter 0 if you don’t mind the deltaG figures not being corrected for magnesium concentration.

Monovalent cation (e.g. sodium) concentration in mM:
: Enter your concentration of sodium etc in nanomoles per cubic metre (decimal fractions are allowed). If in doubt, try 50.

dNTP concentration in mM (0 for no correction):
: Enter your concentration of deoxynucleotide (dNTP) in nanomoles per cubic metre (decimal fractions are allowed). Enter 0 if you don’t mind the deltaG figures not being corrected for dNTP concentration.

(end of deltaG questions)

Shall I count how many pairs have what score/deltaG range? (y/n): 
: Answer “y” if you want a fast summary of how many pairs of primers (in the entire collection, before pooling) have what range of interaction strengths. This could be used for example to check a pool that you have already chosen manually, or if you want a rough idea of the worst-case scenario that pooling aims to avoid.

* If you answered yes to this question, the summary will be displayed on screen, and you will be asked if you also want to save it to a file. If you answer yes to this, you will be asked for a filename.

* These up-front counts will include self-interactions (a primer interacting with itself), and interactions between the pair of primers in any given set. Self-interactions and in-set interactions are _not_ counted when summarizing the counts of each pool (below).

Do you want to see the highest bonds of the whole file? (y/n): 
: Similar to the above question, this can be useful for checking a manual selection or for a rough idea. If you answer Yes, you will be asked for a deltaG or score threshold, and all interactions worse than that threshold will be displayed on-screen with bonds diagrams such as:

     5'-GGCTGAGCGCTCAGTTT-3'
        xx||||||||||||xx
    3'-TTTGACTCGCGAGTCGG-5'

and you will then be asked if you wish to save it to a file, and, if so, what file name. You will then be asked if you would like to try another threshold.

Shall I split this into pools for you? (y/n): 
: Most users will want to say _y_ here, unless you merely wanted to check a batch of primers that you picked some other way. If you say No, Primer Pooler will forget about the primers at hand and ask you if you want to start the program again or exit.

Shall I check the amplicons for overlaps in the genome? (y/n): 
: If you answer yes to this, Primer Pooler will prompt you for a genome file, either in .2bit format as supplied by UCSC, or in .fa (FASTA) format.

To obtain a .2bit file from UCSC:

1. Go to http://hgdownload.cse.ucsc.edu/downloads.html

2. Choose a species (e.g. Human)

3. Choose “Full data set”

4. Scroll down to the links, and choose the one that ends `.2bit` (e.g. [hg38.2bit](http://hgdownload.cse.ucsc.edu/goldenPath/hg38/bigZips/hg38.2bit))

Primer Pooler will then ask "Do you want me to ignore variant chromosomes i.e. sequences with `_` or `-` in their names?" (you'll probably want to answer Yes if you're using hg38.2bit), and will then ask for a maximum amplicon length (in base pairs): this is the maximum length of the _product_—the number does _not_ include the length of any tag sequences you have added to the primers. Then it will scan through the genome data to detect where your amplicons start and finish, and which ones overlap.

* After the overlap scan is complete, Primer Pooler will then have enough data to write an input file for MultiPLX if you wish to run that software as well for comparison. If you decline this, it will ask if you want it to write a simple text file with the locations of all amplicons, which you may accept or decline.

* If you do _not_ opt to check for overlaps in the genome, then Primer Pooler will _not_ take overlaps into account when generating its pools. This is rarely useful unless you have _already_ ensured there are no overlaps in the set of amplicons under consideration. Even then, I would recommend performing a scan anyway, just to double-check: an early version found 11 overlaps in a supposedly overlap-free batch drawn up by an experienced academic—we all make mistakes. But bypassing the overlap check might be useful _if_ you are sure there are no overlaps and you don’t want to download a very large genome file to the workstation you’re using.

How many pools? 
: Enter a number of pools. Before answering this question, you will be given a “computer suggestion”, which is the approximate lowest number of pools needed to achieve no worse than a deltaG of -7 (or a score of 7) in each. _If you’re not sure how many pools, just pick a number and see._ You will be allowed to come back to this question later and try a different number if you weren’t happy with the result.

Do you want to set a maximum size of each pool? (y/n): 
: As the program explains, setting a maximum size of each pool can make the pools more even. If you decide to set a maximum, you will be asked to set the maximum number of primer-sets in each pool. Before answering this question you will be given a computer suggestion and a lower limit.

You will not be allowed to set the maximum size of each pool lower than the average size of each pool, since that would make it logically impossible to fit all primer-sets into all pools. It is not advisable to set it _just above_ the average either, since being overly strict about the evenness of the pools could hinder Primer Pooler from finding a solution with lower dimer formation. You might want to experiment with different maxima—you will be able to come back to this question and try again.

Do you want to give me a time limit? (y/n): 
: If you answer y, you will be asked to set a time limit in minutes. Normally 1 or 2 is enough, although you may wish to let it run a long time to see if it can find better solutions. You don’t _have_ to set a time limit: you may manually interrupt the pooling process at any time and have it give the best solution it has found so far, whether a time limit is in place or not. Additionally, Primer Pooler will stop automatically when it detects better solutions are unlikely to be found.

Do you want my “random” choices to be 100% reproducible for demonstrations? (y/n): 
: If you answer y, Primer Pooler’s random choices will be generated in a way that merely _look_ random but are in fact completely reproducible. This is useful for demonstration purposes—you’ll know how long it will take to find the solution you want. Otherwise, the random choices will be less predictable, as a different sequence will be chosen depending on the exact time at which the pooling was started.

Pooling display
: While pooling is in progress, Primer Pooler will periodically display a brief summary of the best solution found so far, showing the pool sizes, and the counts of interactions (by deltaG range or score) within each pool. As instructed on screen, you may press Ctrl-C (i.e. hold down Ctrl while pressing and releasing C, then release Ctrl) to cancel further exploration and use the best solution found so far.

Do you want to see the statistics of each pool? (y/n): 
: After the pooling is complete, or after you have interrupted it (by pressing Ctrl-C as instructed on screen), you will be asked if you wish to see the interaction counts of _each_ pool (rather than a simple summary of _all_ pools as appeared during pooling). If you want this, you will also be asked if you wish to save them to a file, and, if so, what file name.

Do you want to see the highest bonds of these pools? (y/n): 
: If you answer Yes, you will be asked for a deltaG or score threshold, and all interactions worse than that threshold will be displayed on-screen with bonds diagrams such as:
     5'-GGCTGAGCGCTCAGTTT-3'
        xx||||||||||||xx
    3'-TTTGACTCGCGAGTCGG-5'

and you will then be asked if you wish to save it to a file, and, if so, what file name. You will then be asked if you would like to try another threshold.

Shall I write each pool to a different result file? (y/n): 
: If you answer _y_ to this, you will be asked for a prefix, which will be used to name the individual results files. Otherwise, you will be asked if you wish to save all results to a single file. If you decline saving all results to a single file, the results will not be saved at all—this is for when you weren’t happy with the solution and want to go back to try a different number of pools or a different maximum pool size.

Do you want to try a different number of pools? (y/n): 
: This question is self-explanatory. You can go back as many times as you like, trying different numbers of pools. But many researchers have a pretty good idea of how many pools they want to use, or else are happy with the computer’s initial suggestion.

Would you like another go? (y/n): 
: If you answered No to trying a different number of pools, or if you didn’t want the program to do pooling at all, then you will be asked if you want to start the program again.  Answering No to this question will exit.

Command-line usage
------------------

Besides running interactively (see above), it is also possible to run Primer Pooler with command-line arguments. This section assumes familiarity with the concept of running programs from the command line.

The only _mandatory_ argument (if not running interactively) is a filename for the primers file. This should be a text file in multiple-sequence FASTA format, such as:

    >toySet1-F
    AGCTGCTGCTGCGATCT
    >toySet1-R
    GGCTGAGCGCTCAGTTT
    >toySet2-F
    ACGGCTTGACACCGTTCGACTG
    >toySet2-R
    CAGACGTTCAG

(this example does not represent real primers). Degenerate bases are allowed using the normal letters, and both upper and lower case is allowed. Names of amplicons’ primers should end with F or R, and otherwise match. Optionally include tags (tails, barcoding) to apply to all primers: >tagF and >tagR (tags can also be changed part-way through the file).

Processing options should be placed before this filename. Options are as follows:

`--help` or `/help` or `/?`
: Show a brief help message and exit.

`--counts`
: Show score or deltaG-range pair counts for the whole input. deltaG will be used if the `--dg` option is set (see below). This option produces a fast summary of how many primer pairs (in the entire collection, before pooling) have what range of interaction strengths. This could be used for example to check a pool that you have already chosen manually, or if you want a rough idea of the worst-case scenario that pooling aims to avoid.

`--self-omit`
: Causes the `--counts` option to avoid counting self-interactions(a primer interacting with itself), and interactions between the pair of primers in any given set.

`--print-bonds=THRESHOLD`
: Similar to `--counts`, this can be useful for checking a manual selection or for a rough idea. All interactions worse than the given threshold (deltaG if `--dg` is in use, otherwise score) will be written to standard output, with bonds diagrams.

`--dg[=temperature[,mg[,cation[,dNTP]]]]`
: Set this option to use deltaG instead of score. Optional parameters are the temperature (default is human blood heat), the concentration of magnesium (default 0), the concentration of monovalent cation (e.g. sodium, default 50), and the concentration of deoxynucleotide (dNTP, default 0). Decimal fractions are allowed in all of these. Temperature is specified in kelvin, and all concentrations are specified in nanomoles per cubic metre.

`--suggest-pools`
: Outputs a suggested number of pools. This is the approximate lowest number of pools needed to achieve no worse than a deltaG of -7 (or a score of 7) in each.

`--pools[=NUM[,MINS[,PREFIX]]]`
: Splits the primers into pools. Optional parameters are the number of pools (if omitted or set to `?` then the suggested number will be calculated and used), a time limit in minutes, and a prefix for the filenames of each pool (set this to `-` to write all to standard output).

`--max-count=NUM`
: Set the maximum number of pairs per pool. This is optional but can make the pools more even. A maximum lower than the average is not allowed, and it’s usually best to allow a generous margin above the average.

`--genome=PATH`
: Check the amplicons for overlaps in the genome, and avoid these overlaps during pooling. The genome file may be in .2bit format as supplied by UCSC, or in .fa (FASTA) format.

`--scan-variants`
: When searching for amplicons in a genome file, scan variant sequences in that file too, i.e. sequences with `_` and `-` in their names.  By default such sequences are omitted as they're not normally needed if using hg38.

`--amp-max=LENGTH`
: Sets maximum amplicon length for the overlap check. The default is 220.

`--multiplx=FILE`
: Write a MultiPLX input file after the `--genome` stage, to assist comparisons with MultiPLX’s pooling etc.

`--seedless`
: Don’t seed the random number generator

`--version`
: Just show the program version number and exit.

Changes
-------

Version 1.0 had important bugs that can affect results:

1. an error in incremental-update logic sometimes had the effect of generating suboptimal solutions (in particular, pools could be unnecessarily empty, and/or full beyond any limit that was set);

2. an error in the user-interface loop meant that if you use tags, run interactively, and answer “yes” to the question “Do you want to try a different number of pools”, the _second_ run will have been done without the tags, and its results will have been de-tagged _twice_, removing some bases from the output; moreover, the resulting truncated versions of your primers will have made it into the interaction calculations for any third run.

These bugs have now been fixed. In addition, Versions 1.1 through 1.13 had a bug related to the first fix, which would cause interaction-checking for pooling purposes to be performed _without_ tags when running in interactive mode (command-line mode was not affected). I therefore recommend re-running in the latest version.

Versions prior to 1.17 also had a display bug: the concentrations for the deltaG calculation are in millimoles per litre, not nanomoles as stated on-screen in interactive mode (please ignore the on-screen instruction and enter millimoles, or upgrade to the latest version which fixes that instruction).

Versions prior to 1.34 would round down any decimal fraction you type when in interactive mode (for deltaG temperature, concentration and threshold settings). Internal calculation and command-line use was not affected by this bug.

Versions prior to 1.37 did not ignore whitespace characters after FASTA labels.

Version 1.2 added the MultiPLX output option, and Version 1.33 fixed a bug when MultiPLX output was used with tags and multiple chromosomes. Version 1.3  added genome reading from FASTA (not just 2bit), auto-open browser, and suggest number of pools.

Version 1.36 clarified the use of Taq probes, and allowed these to be in the input file during the overlap check. It’s consequently stricter about the requirement that reverse primers must end with `R` or `B`: previous versions would accept any letter other than `F` for these.

Version 1.4 allows tags to be changed part-way through a FASTA file. For example, if there are two `>tagF` sequences, the first `>tagF` will set the tags for all `F` primers between the beginning of the file and the point at which the second `>tagF` is given; the second `>tagF` will set the tags for all `F` primers from that point forward. You can change tags as often as you like.

Version 1.5 allows primer sets to be “fixed” to predetermined pools by specifying these as primer name prefixes, e.g. `>@2:myPrimer-F` fixes `myPrimer-F` to pool 2.

Version 1.6 detects and warns about alternative products of non-unique PCR.  It was followed within hours by Version 1.61 which fixed a regression in the amplicon overlap check.

Version 1.7 makes the ignoring of variant sequences in the genome optional, and warns if primers not being found might be due to variant sequences having been ignored.

Version 1.72 changes the license to Apache 2.0.

Glossary
--------

Base
: The nitrogenous base part of a nucleotide in a DNA sequence, represented by `A`, `C`, `G` or `T`. Informally, “base” can also be used to refer to the entire nucleotide.

Complement
: What the base binds with. `T` binds with `A` and `C` binds with `G`. Complementing a sequence means swapping A for T and C for G throughout.

Degenerate base
: A base we’re not sure about because of genetic variation in a population. We can use extra letters to specify which bases are allowable.

IUPAC/IUBMB degenerate-base codes:

    K = G or T
    Y = C or T
    S = C or G
    W = A or T
    R = A or G
    M = A or C
    B = any except A
    D = any except C
    H = any except G
    V = any except T
    N = any

Primer _or_ Oligo
: A short string of bases (actually nucleotides) that’s used to start copying from the strand of DNA we’re testing. The primer matches up with the start of a section of DNA we want to copy. There are also extra structures at the two ends of the primer that set its direction: these are written as `5'` (for the phosphate start) and `3'` (for the hydroxyl end). The actual copying occurs from the _complementary_ strand, but we can ignore this. Primers are special cases of molecules called oligonucleotides.

Degenerate primer
: A primer that has one or more degenerate bases. In practice, this means we manufacture separate primers for each combination of allowable bases and mix them together. So we have to make worst-case assumptions about these when checking for dimers or overlaps.

Amplicon
: A section of the DNA we’re interested in amplifying (producing copies of). Primers are designed to copy it.

Primer set
: Two primers, corresponding to the start and end of an amplicon. They must be kept in the same pool. Sometimes called a “primer pair”, but this might be confused with the two participants of a _dimer_ (below) so I think “set” is better. The two primers in a set are called “forward” and “reverse” primers, but the reverse primer is _not_ a backward copy of the forward one—if you’re reading my code, you have to be aware of the distinction between backward, which is just a flipped-over copy of any sequence, and reverse, which is the second primer of a set.  With assistance from an enzyme called polymerase, the forward primer begins copying from the start of the amplicon, while the reverse primer begins from the end of the amplicon. Although these initial copies continue for an indeterminate number of bases (probably not the whole chromosome, but longer than the region we want), the _second_ cycle will apply the forward primer to the ‘end’ section of what the reverse primer produced, and conversely the reverse primer to the ‘start’ section of what the forward primer produced, in both cases resulting in exactly the amplicon we want (which is then reduplicated in subsequent cycles).

Negative strand
: The complement of the normal (positive) sequence in the genome. If a primer is designed to match the negative strand then you need to complement it and read it backwards to match the (positive) genome data. In a set, _one_ of the two primers will be a negative-strand primer, but the primer file won’t tell us which one (it’s _not necessarily_ the “reverse” primer: when a chromosome has a gene on its negative strand, primers are typically labelled in the other direction so we’ll see the “reverse” primer on the positive strand followed by the “forward” primer on the negative). You can’t put both primers on the _same_ strand because collisions would occur during copying.

Pool _or_ Subpool _or_ Group _or_ Tube _or_ Primer set combination (PSC)
: A bunch of primer-sets all drifting around in the same mixture. When that mixture is added to some of our sample of DNA, the amplicons whose primer-sets are in that pool are copied (amplified) so we can measure them. If we can reduce the number of different pools we need, we can finish the testing more quickly and use up less of the sample, but on the other hand we want to avoid combinations that overlap or form dimers.

Overlap
: Two primer-sets that access overlapping sections of the genome. If they are placed in the same pool, an unwanted shorter amplicon is produced. Consider the following toy example:

    ....1..2..3..4....
        A-----B
           C-----D
           C--B

If primers `A` and `B` are designed to obtain an amplicon from position `1` to `3`, and `C` and `D` are designed to obtain an amplicon from `2` to `4`, then placing them in the same pool will result in excessive pairings between `C` and `B`, producing a short amplicon from `2` to `3` at the expense of the other two. This is very bad news and we have to pick our pools to avoid it.

Dimer
: Two primers stuck to each other. This is bad news because, if they’re stuck to each other, they’re not helping us test the sample.  But a dimer is not as bad as an overlap: just because two primers _can_ form a dimer doesn’t mean they _will_, and the experiment might run anyway on the fraction of primers that didn’t get stuck. But it’s _better_ if each pool can have a combination of primers that tends to produce as few dimers as possible.

Score
: A number that gives a rough idea of how likely it is that two primers will make a dimer. It’s just the number of bases that bond, minus the number of bases that don’t, and ignoring any bases that are left dangling off either end. This is repeated for all positions and the worst case is taken.

Delta G (dG)
: The change in Gibbs free energy when two primers make a dimer. The more negative this is, the more likely dimers will form. This thermodynamics calculation gives better results than score, while being only a _little_ slower (unless you have ridiculous numbers of degenerate bases). It does need to know the temperature and amounts of various chemicals, but if you don’t know these, the defaults should still be reasonable for comparisons.

Genome
: _All_ the DNA in the cell (most species have hundreds of megabytes at the very least). We need data about the whole genome to work out which amplicons will overlap. If some parts are still unknown, we ignore those and hope for the best.

Tag _or_ index sequence _or_ barcode _or_ tail
: A constant set of extra bases added to the beginning (`5'`—actually the _end_ on the complimentary strand) of every forward or reverse primer. This is used for fishing the results out of the pool. If you tell Primer Pooler what tags you are using, it takes them into account when checking for dimers, while ignoring them when checking the genome for amplicon overlaps.

Efficiency
: The rate at which amplicons are copied, as a fraction of the ideal rate. Particularly important in quantitative PCR (qPCR) as you need to know the copy rate for the final counts to be meaningful. Efficiency is improved with dimer reduction, but it can also depend on manufacturing quality and equipment quality, so each batch needs to be checked experimentally.

Massive(ly) parallel sequencing _or_ next-generation sequencing _or_ second-generation sequencing _or_ high-throughput sequencing
: Base-by-base reading of thousands of short sections of a genome in parallel. Less expensive machines in smaller labs typically need the relevant sections of the genome to be amplified first. If a reference copy of the genome has already been sequenced and we want to re-sequence specific sections to check them for alterations, then we can use multiplex PCR to pull out these sections. This may involve dealing with far more amplicons than is the case with PCR for detecting or counting genes.

AutoDimer
: A 2004 program to check a single pool for dimers. AutoDimer was coded in Visual Basic 6 and its dimer search is several thousand times slower than Primer Pooler’s; re-pooling must be done manually, as must the handling of degenerate bases.

Thresholding
: A simple and fast way of grouping primer sets: “don’t add a set to a pool if the interaction badness would exceed some threshold” (usually dG&lt;-7 or overlap). The total number of pools required is discovered by the computer, not chosen by the user. Primer Pooler uses thresholding to _suggest_ a number of pools, but allows the user to override it for minimisation.

Minimisation
: Method used by Primer Pooler to group primer sets into a user-specified number of pools, seeking to minimise the interactions within each pool.

MPprimer
: A 2009 GPLd Perl+Python program for finding optimal PSCs by thresholding. Slower than our C bit-patterns code and cannot cope with degenerate primers.

MultiPLX
: A 2004 C++ program for grouping primer-sets by thresholding.  No overlap checking:  you are expected to divide the batches yourself and run them separately. MultiPLX can score on differences between melting temperatures, and also on unwanted extra interactions between primer and product-amplicon (which isn’t normally a concern when large numbers of primers are involved); its interaction calculations are slower than ours and it makes up for this by giving you the option of not checking for _every_ kind of interaction. Primer Pooler has an option to output your primers and their products (after genome search) in MultiPLX’s input format if you wish to compare with MultiPLX’s scoring.

Bit patterns
: A computer programming technique that involves writing information about different items into different binary digits of the same number, loading that number into the computer’s calculation circuitry, and getting it to do something to all its digits in one operation, thus processing many items together. This is even more effective on newer CPUs, because their wider registers can take even more digits at a time. Primer Pooler uses bit-pattern techniques for its bonding calculations.

C compiler
: A computer program that takes something written in the C programming language and converts it into machine code that the CPU can run quickly. Modern C compilers can be _frighteningly_ good at this, so a well-written C program can easily outpace what can be done in more “beginner-friendly” languages. This doesn’t usually matter if you just want to show things on the screen and wait for input, but you _will_ notice the difference when big calculations are involved.

C++
: A computer language inspired by C but with many extra features which, if used well, can make programs easier to manage. In theory, well-written C++ can equal the speed of well-written C. In practice there can be problems with some C++ compilers. Since I was handling register-level bit patterns and builtins for specific CPU opcodes, I decided not to risk it and stick with C even though I _could_ have done it in C++.

Command line
: A way of interacting with the computer that involves typing commands on the keyboard and seeing the computer’s response written below. It might not look as nice as a modern graphical desktop, but it can be quite efficient when you get used to it; moreover, if you’re writing in C then the command line tends to be the easiest interface to write for, freeing up the programmer to concentrate on the calculation part instead of having to spend all their time making it look pretty. Sometimes _another_ programmer who specialises in pretty front-ends will come along later and add one. (I’m more of a “back-end” than a “front-end” programmer.)

CRISPR
: Naturally occuring DNA fragments in unicellular immune systems that have been repurposed for genetic engineering. Widely hailed as the “next big thing” after PCR, but doesn’t yet replace it in all cases.  CRISPR is more about editing genes like a Unix `sed` command (you script the edits but don’t see them happen), but it can be modified to create a visible signal when a cut is made, thereby becoming a sequence-detection tool for one sequence at a time.

Citation
--------

Silas S. Brown, Yun-Wen Chen, Ming Wang, Alexandra Clipson, Eguzkine Ochoa, and Ming-Qing Du (2017).
PrimerPooler: automated primer pooling to prepare library for targeted sequencing.
Biology Methods and Protocols. Oxford University Press. 2(1).
doi:[10.1093/biomethods/bpx006](http://doi.org/10.1093/biomethods/bpx006)

License
-------

Primer Pooler is free software, now licensed under the Apache License, version 2.0.
Prior to v1.72 it was licensed under the GNU General Public License,
version 3 or later; the new Apache 2 license is still GPL-compatible but with
added permissions to make it more acceptable in laboratories with blanket legal
policies against GPL'd code.

Thanks
------

I’ve lost track of how many giants I’ve stood on the shoulders of for this, but they include:

* All the scientists who figured out how DNA works and sequenced the human genome;

* Martin Richards for his BCPL bit-pattern techniques, which influenced the way I wrote the fast dimer check;

* The free/libre and open source software community for their legal research, a C compiler, editor and debugger;

* my wife Yun-Wen, who needed this for her cancer-research project, provided test data and feedback, and put up with all my silly questions.

Typescript player for HTML5
===========================

I wrote this to make an in-browser demonstration of Primer Pooler. It uses code from DOStoy to display a typescript in an HTML5 canvas.

Advantages:

* The Javascript is entirely self-contained. It does not require any special setup on the server side and you don't have to rely on a third-party server.

* Reduces data transfer without blurring the text (although HTML5 Canvas can be blurred under _some_ circumstances)

* New text stays entirely within the canvas and is not added to the web page, thus avoiding bad interactions with screenreaders and accessibility CSS

* Canvas appears only on wide screens, won't clutter mobile devices (I'm assuming the demonstration is non-essential)

* Animation does not start until clicked on (to avoid annoying anyone who's not looking at it), and can be paused/resumed at any time by clicking again

* The typescript can be viewed in the Javascript source.

Disadvantages:

* No rewind control

* Won't stop the screensaver

* Can't copy/paste or change the font without looking at the source

* The terminal program must be simple: think “line-mode interaction with some colours”. Full-screen editors etc will _not_ work properly (at least not without further work on the converter)

* No screen-reader accessibility. But as mentioned above I'm assuming the demonstration is entirely optional; people who don't think visually shouldn't need it (I know I don't; I'm just doing this because apparently some sighted people want it).

License: MIT

Usage:

1. On a GNU/Linux terminal, do `script -t log2 2>log1` and record as appropriate

2. Do `python script2canvas.py >demo.js`

3. Optionally edit the resulting `demo.js` file if you want to ‘airbrush’ your mistakes

4. In your HTML, put:

    <span id="DOStoyPlace"></span><script src="demo.js"></script>

5. Test

Copyright and Trademarks
========================

All material © Silas S. Brown unless otherwise stated.  Licenses as stated above.

* Apache is a registered trademark of The Apache Software Foundation.

* BLAST is a registered trademark of the National Library of Medicine.

* Javascript is a trademark of Oracle Corporation in the US.

* Linux is the registered trademark of Linus Torvalds in the U.S. and other countries.

* Any other trademarks I mentioned without realising are trademarks of their respective holders.
