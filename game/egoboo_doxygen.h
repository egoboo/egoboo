#pragma once

//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

// This file is only used to set-up some special features of the doxygen automatic
// source code documentation. This file uses the doxygen's normal "\" notation. In the rest
// of the project, the "@" notation is used to avoid confusion about slashes

/**
 * \mainpage Egoboo
 * \version
 *
 * <P>Thank you for your interest in Egoboo!</P>
 *
 * <P>Egoboo is an open source, cross-platform, community development computer
 * game.  It is intended to be a 3D role playing game in the spirit of
 * NetHack and other games of the roguelike genre.   The current game is
 * completely playable from beginning to end, and could be considered
 * &quot;finished&quot;.</P>
 *
 * <P>If you are reading this, it is likely that you are interested in modding
 * Egoboo's code.  Good, because we could use the help! As of
 * 12-31-2009, the current code-base is maintained on an SVN  server at
 * SourceForge, <A HREF="https://egoboo.svn.sourceforge.net/svnroot/egoboo/branches/2.6.x">here</A>.
 * The code in the trunk is actually an experimental version with more
 * &quot;advanced&quot; code that may be migrated into the current code.</P>
 *
 * <H3>Coding Style</H3>
 * <P>You will find a complete mish-mosh of programming standards in the code.
 * These are the result of a long history and a lot of different
 * developers. The preferred coding style is</P>
 *
 * <UL>
 *  <LI><P>Use our variant of the ansi-coding style, where opening brackets are
 *  placed on their own line (except in the case of single-line function
 *  blocks), etc. You can call the run_astyle.sh macro or call &quot;astyle
 *  --mode=c -s4bCSKNwm4pDUoOcZ&quot; to achieve the formatting used in
 *  the code</P>
 *
 *  <LI><P>When
 *  naming variables or functions, avoid CamelCase naming in favor of
 *  using underscores to separate  multi-word identifiers.</P>
 *
 *  <LI><P>When creating data structures, look forward to the conversion of the
 *  program to C++. This means that you should provide a default
 *  initialization function, and a function to delete any dynamically
 *  created data housed within the structure. You should also use a
 *  naming convention that is consistent with C++.  For instance, if we
 *  had started with the C++ class foo, and the function foo::bar(), we
 *  could say that the C function name should be foo_bar(). So, just
 *  name everything by the convention module_function_name() or
 *  struct_type_function_name(), so that the conversion to C++ will go
 *  as smooth as possible.</P>
 *
 *  <LI><P>Please respect that certain functions and modules will not be converted to
 *  C++. These modules have not been marked yet, but they are : enet,
 *  and the file_formats/id_*.* md2 loading code, and the extensions/*.*
 *  code. This also includes (to a lesser degree) the the
 *  file_formats/configfile.* module. The basic reason is  that this
 *  code is being used in multiple projects that are intended to share
 *  the code. If you must, do something to upgrade the code, make
 *  wrapper code. :)</P>
 * </UL>
 *
 * <P> As mentioned above, a lot of the code organization is completely
 * accidental and historical. The original code was developed by Aaron
 * Bishop (under the pseudonym Programmer X) in C using DirextX for
 * graphics. It was not modular, did not use structs, statically
 * allocated all game resources, and was contained in one large header
 * and one large .c file. Much of this has been modded over the years to
 * conform to modern programming practice. However certain things, like
 * converting the program over to  using dynamic allocation for game
 * resources, have been put off until later. Also, certain code that was
 * contributed by Jonathan Fisher uses a significant amount of CamelCase
 * and had not been converted to the current coding convention, mostly
 * out of respect of him developing that code for us, I think! ;)</P>
 *
 * <P>Since this is a community development project, it is pretty important to
 * respect the other developers on the project. You are going to find a
 * bunch of tags in the code which label the contributions of various
 * developers.</P>
 *
 * <UL>
 *  <LI><P>ZZ&gt;
 *  = &quot;Programmer X&quot; = &quot;Aaron Bishop&quot;</P>
 *  <LI><P>JF&gt;
 *  = &quot;Jonathan Fischer&quot;
 *  </P>
 *  <LI><P>ZF&gt;
 *  = &quot;Zefz&quot; = &quot;Johan Jansen&quot;
 *  </P>
 *  <LI><P>PF&gt;
 *  = &quot;Penguinflyer5234&quot; = &quot; n/a&quot;
 *  </P>
 *  <LI><P>BB&gt;
 *  = &quot;Ben Birdsey&quot;</P>
 * </UL>
 *
 * <P>We have been very careful to maintain a proper &quot;chain of
 * development&quot;. For instance, when refactoring a single  code
 * block into multiple functions, the new functions are given the same
 * tag as the original developer. If any new code was added or modded,
 * then the new developer's tag is added after the original with its own
 * set of notes.</P>
 *
 * <P>As a last note, I cannot encourage the use of comments in code enough!
 * Some of the original Programmer X code was very difficult to
 * understand, and it was only with the help of multiple people's
 * analysis and comments that it was possible to decypher the
 * side-effects of changing certain code blocks. Please do not let the
 * code get into that state again!</P>
 *
 * <H3>Directions</H3>
 * <P>Are you looking for something to do? Well, the code could always use
 * optimization and better documentation. </P>
 *
 * <P>There are several large projects that could definitely benefit the project:</P>
 *
 * <UL>
 *  <LI><P>Conversion of various game systems to more modern algorithms. Many of these
 *  have been  implemented in the trunk version and could be ported over
 *  into the current program. For instance,  the octree collision
 *  detection.</P>
 *
 *  <LI><P>The program is currently implemented using a lot of roll-your-own code
 *  to run on SDL with OpenGL. Personally, I would like to see a
 *  migration to something like irrLicht or another game engine that has
 *  a much faster architecture.</P>
 *
 *  <LI><P>The ability to load more than one type of character model (md3, ms3d,
 *  etc.) and the ability to  load up more than one terrain mesh (i.e.
 *  for the buildings to be loaded as separate meshes)</P>
 *
 *  <LI><P>Full implementation of randomly generated meshes / modules</P>
 *
 *  <LI><P>Full conversion to some kind of &quot;normal&quot; scripting language
 *  like Lua</P>
 *</UL>
 *
 * <H3>Philosophy</H3>
 * <P>There has been a lot of discussion about the development of Egoboo, and
 * some of it has been pretty heated! Well, what would you expect when
 * you have a bunch of passionate developers?</P>
 *
 * <P>In any case, we have always fallen back to two questions: </P>
 * <OL>
 *  <LI><P>What would make this more rogue-like?  </P>
 *  <LI><P>Did Aaron say anything about it in his dev documents?  </P>
 * </OL>
 *
 * <P>Thankfully, that has settled most arguments! There are still some open questions
 * about the actual play-style of the game (like whether it is an
 * arcade-style game), how to implement side-quests, whether it is
 * important to have a central narrative in a hack and slash, etc.</P>
 *
 * <P>I would encourage you to do a little research on Rogue-likes and their
 * history before diving into full-on game design. So much of this
 * territory has been explored before that you are likely to  copy
 * someone by accident!</P>
 *
 * <H3>License</H3>
 * <P>Egoboo is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your
 * option) any later version.</P>
 *
 * <P>Egoboo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.</P>
 *
 * <P>You should have received a copy of the GNU General Public License along
 * with Egoboo.  If not, see <A HREF="http://www.gnu.org/licenses/">http://www.gnu.org/licenses/</A>.</P>
 **/