<a name="top"> </a>

[This document is formatted with GitHub-Flavored Markdown.              ]:#
[For better viewing, including hyperlinks, read it online at            ]:#
[https://github.com/sourceryinstitute/opencoarrays/blob/3rd-party-tools/open-fortran-parser-sdf/trans/README.md]:#


Coarray Fortran Emulation (CAFe) Transpiler
===========================================

* [OverView](#cafe)
* [Transformation Strategies](#transformation-strategies)
* [Examples](#examples)

Overview
--------
[CAFe] has dual meaning: it refers to the source-to-source transformation compiler ("transpiler") that transforms Fortran 2015 coarray parallel programming syntax into Fortran 2003 features that at least a half-dozen recent Fortran compilers support. CAFe also refers to a set of CAF extensions through which the CAFe transpiler supports emerging many-core and hetereogeneous processors and accelerators.

Transformation Strategies
---------

The fast-to-ocaf.str document defines the CAFe transformation strategies using StrategoXT.

For documentation purposes and in the interest of inviting contibutions, this README file describes some of the StrategoXT syntax:

*  ? match operator

*  ! produces a term

*  | separates preceding strategies from following arguments


Examples
-------------
The OpenCoarrays test suite contains several sample CAF codes that were used to develop the initial set of transformation rules.


[Hyperlinks]:#

[Overview]: #overview
[Source Transformation]: #downloads
[Examples]: #compatibility
