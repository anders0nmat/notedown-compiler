# Notedown

- [Notedown](#notedown)
	- [Description](#description)
	- [Featureset](#featureset)
		- [Headings](#headings)
		- [Paragraphs](#paragraphs)
		- [Text Styling](#text-styling)
		- [Escape Sequences](#escape-sequences)
		- [Blockquotes](#blockquotes)
		- [Ordered and unordered lists](#ordered-and-unordered-lists)
		- [Code](#code)
		- [Horizontal Line](#horizontal-line)
		- [Links](#links)
		- [Images](#images)
		- [Tables](#tables)
		- [Fenced Code Blocks](#fenced-code-blocks)
		- [Footnotes](#footnotes)
		- [Heading IDs](#heading-ids)
		- [Definition Lists](#definition-lists)
		- [Task Lists](#task-lists)
		- [Emoji](#emoji)
		- [Highlight](#highlight)
		- [Reference-Style Links](#reference-style-links)
		- [Syntax Highlighting](#syntax-highlighting)
		- [Links to Headings](#links-to-headings)
		- [HTML Tag support](#html-tag-support)
		- [Admonitions](#admonitions)
		- [Commands](#commands)
		- [Better table formatting](#better-table-formatting)
		- [Available command functions](#available-command-functions)
		- [Collapsible Headings](#collapsible-headings)
		- [Replace Content](#replace-content)
		- [Types of Links](#types-of-links)

## Description

Notedown is a Quick-HTML-Language heavily inspired by Markdown. It extends on the concepts and removes repeating patterns. The concept is a DRY (Dont Repeat Yourself) language (So there should be only one way to reach a specific goal, while concepts keep consistent), which is optimal for taking notes and experimenting with ideas, while having easy syntax which is also readable without compiling.

## Featureset

Status:  
0% - not started  
untested - Implemented, but not thoroughly tested  
complete - Tested and working (as expected)

- Markdown Features
  - Headings (untested)
  - Paragraphs (untested)
  - Text Styling (bold, italic, underlined, strikethrough) (untested)
  - Blockquotes (untested)
  - Ordered and Unordered Lists (50%)
  - Code (untested)
  - Horizontal Line (complete)
  - Links
  - Images
- Extended Markdown Features
  - Tables
  - Fenced Code Blocks
  - Footnotes
  - Heading IDs
  - Definition List
  - Task List
  - Emoji
  - Highlight (untested)
  - Reference-Style Links
  - Syntax Highlighting
  - Links to Headings
  - HTML Tag Support
  - Admonitions
- My Contribution
  - Element Commands
  - Better Table Formatting
  - Collapsible Headings
  - Replace Content

### Headings

Headings should function like in regular Markdown. They are indicated by hashtags (#) on an empty line. One can create six different headings, with the respective amount of hashtags.

Input:
```md
# Heading 1

## Heading 2
```

Output:
```html
<h1 id="heading-1">Heading 1</h1>

<h2 id="heading-2">Heading 2</h2>
```

### Paragraphs

Paragraphs are separated by an empty line, identical to regular Markdown.

Input:
```md
Text in Paragraph 1
Still in Paragraph 1

Text in Paragraph 2
```

Output:
```html
<p>Text in Paragraph 1
Still in Paragraph 1</p>

<p>Text in Paragraph 2</p>
```

### Text Styling

Text Styling uses simple Symbols. They have to be immediately followed by (or predecessed by) a non-whitespace-character to be considered valid. If there is any wrongly nested styling symbols (or other reasons foramtting symbols are invalid) they should be outputted as is.

Input:
```md
*bold*
/italic/
_underlined_
~strikethrough~

*/_all three at once_/*

*_Invalid Styling*_
```

Output:
```html
<b>bold</b>
<i>italic</i>
<u>underlined</u>
<s>strikethrough</s>

<b><i><u>all three at once</u></i></b>

*_Invalid Styling*_
```

### Escape Sequences 

Escape Sequences are indicated by a `\` and escape all other characters with meaning. If neccessary they are inserted by their HTML-Code. Valid escape characters are:

Character Sequence | Meaning
--|--
\n|Newline character
\\\\ |Backslash
\\*|Asterisk
\\# | Hashtag


### Blockquotes

Blockquotes should work like in Markdown. Nested blockquotes are valid. Two blockquote indicators immediately after one another indicate a centered quote.

Input:
```md
> Normal Blockquote

> Blockquote with paragraphs
> 
> Paragraph 2
> > Nested Blockquote

>> Centered Quote
```

Output:
```html
<blockquote>Normal Blockquote</blockquote>

<blockquote>
<p>Blockquote with paragraphs</p>

<p>Paragraph 2
<blockquote> Nested Blockquote</blockquote>
</p>
</blockquote>

<blockquote class="center">Centered Quote</blockquote>
```

### Ordered and unordered lists

They shall also work as in default Markdown.

Input:
```md
- Entry 1
- Entry 2
 - Entry 2.1
- Entry 3

1. Entry 1
2. Entry 2
 1. Entry 2.1
1. Entry 3
```

Output:
```html
<ul>
<li>Entry 1</li>
<li>
	Entry 2
	<ul>
		<li>Entry 2.1</li>
	</ul>
</li>
<li>Entry 3</li>
</ul>

<ol>
<li>Entry 1</li>
<li>
	Entry 2
	<ul>
		<li>Entry 2.1</li>
	</ul>
</li>
<li>Entry 3</li>
</ol>
```

### Code

Code shall also work similar to Markdown.

Input:
```md
`int main`
```

Output:
```html
<pre><code>int main</code></pre>
```

### Horizontal Line

Horizontal Lines are inserted by three `-` on an empty Line

Input:
```md
Hello World
---
This is important
```

Output:
```html
Hello World
<hr>
This is important
```

### Links

Links are similar to Markdown.

Input:
```md
[Wikipedia](https://www.wikipedia.org :"A description when you hover over")
```

Output:
```html
<a href="https://www.wikipedia.org" title="A description when you hover over">
Wikipedia
</a>
```

If you leave the square brackets empty, the Link will be used as Text to display:

Input:
```md
[](google.com : Goes to google)
```

Output:
```html
<a href="google.com" title="Goes to Google">google.com</a>
```

You can link to other ids in the document automatically by following the square brackets with a `#`:

Input:
```md
[Section Two]#()
```

Output:
```html
<a href="#section-two">Section Two</a>
```

Normal Links in plain text will **not** be parsed to links automatically.

### Images

Images are similar to Links but have a `!` between both parentesis blocks.

Input:
```md
[Image of a Cat]!(/images/kitten.png : Hover Text)
```

Output:
```html
<img src="/images/kitten.png" alt="Image of a Cat" title="Hover Text">
```

### Tables

Tables are indicated by a pipe character `|`.

```md
| first cell | second cell
| :-- | :--:
| first entry | second entry
```

### Fenced Code Blocks

Similar to Markdown.

````
```language
void name(int abc) {
	if (abc == 2)
		throw "Prime";
	return;
}
```
````

### Footnotes

See [Element Commands](#commands)

### Heading IDs

See [Element Commands](#commands)

### Definition Lists

```md
? Word 1
  ! Explanation 1
? Word 2
? Word 3
  ! Explanation 2
? Word 4
  ! Explanation 3
  ! Explanation 4
```

### Task Lists

Task list syntax as in Markdown

```md
- [] Unchecked Entry
- [ ] Unchecked Entry
- [  ] (Not valid)
- [x] Checked Entry
- [X] Checked Entry
```

### Emoji

Emoji can be inserted by enclosing their name with `:`. 

### Highlight

Similar to [Text Styling](#text-styling) but with `=`. Highlights the text.

Input:
```md
There is something =important= among us.
```

```html
There is something <mark>important</mark> among us.
```

### Reference-Style Links

See [Replace Content](#replace-content)

### Syntax Highlighting

Whoo, this will be quite alot of work

### Links to Headings

This should already work if you use `#section-id` as a url in a link. See [Links](#links)

### HTML Tag support

Although it would be nice to use them in plain text, they can be inserted by command (See [Commands](#commands)).
Possibly like `[]{$html ul li}` or `{$html /ul}`

### Admonitions

Admonitions are Segments to be highlighted (such as hints, warnings or errors).
They can be achived in normal Markdown with `> WARN : This is a Warning`, but are not different from other Quotes:

> WARN : This is a Warning

I want to make them stand out more.

Input:
```md
>info> This is a Information

>warn> This is a Warning

>error> This is a Error
```

Output:
```html
<blockquote class="info">This is a Information</blockquote>

<blockquote class="warn">This is a Warning</blockquote>

<blockquote class="error">This is a Error</blockquote>
```

And if you want the default Icon for these to precede your message, 

Input:
```md
>:i:> This has an icon in front of it
```

Output:
```html
<blockquote class="info">ℹ️ This has an icon in front of it</blockquote>
```

### Commands

Throughout this Document square brackets indicated that the Text inside got handled differently. Mostly followed by normal brackets containing information as to how the text is to be understood. So far we had:

`[Links](url.com :"Hover Text")`
`[Pictures]!(url/pic.png :"Hover Text")`

To further extend the language i propose another set of Brackets: curly brackets.

Curly Brackets indicate special commands such as inline css, content generation or direct html manipulation.

Use them with a Text to apply to (so with square brackets up front) or without to apply to the current element.

```md
There is quite some [red]{>color:red >"margin:0 1em 4px"} in there.
```

Output:
```html
There is quite some <span style="color: red">red</span> in there.
```

Available commands are:

- `#<id>` : Adds the html attribute 'id' to the element and sets it to the value of `<id>`. Can **not** appear more than once.
- `.<class>` : Adds the html attribute 'class' and sets it to the value(s) of `<class>`. Multiple classes can be assigned by multiple dots: `.red.center`. Can **not** appear more than once.
- `:"<title>"` : Adds `<title>` as hover text to the element. Can **not** appear more than once.
- `+<name>="<value>"` : Adds `<name>` as attribute to the element and sets it to `<value>`. *Can* appear more than once but `<name>` must be different.
- `$<function> [<args>]` : Calls the predefined `<function>` to further process this Command, see [Available command functions](#available-command-functions). . Can **not** appear more than once.
- `><name>:<value>` : Sets the element style attribute `<name>` to `<value>`. *Can* appear more than once. 

Any of these can be chained together separated by spaces:

Input:
```
Here we have [Complex Text]{#complex .highlight.center :"Discover!" +count=4 $invert_case >color:red}
```

Output:
```html
Here we have <span id="complex" class="highlight center" title="Discover!" count="4" style="color:red">
cOMPLEX tEXT
</span>
```

### Better table formatting

See [Replace Content](#replace-content)

### Available command functions

- `insert_toc` : Inserts the table-of-contents
- `no_toc` : excludes the current heading from the table-of-contents
- `now <format>` : Inserts the current date and time. A formatting can be specified with the `<format>` arg. See []()

### Collapsible Headings

One can define a collapsible section with a `+-- [...] --+` section.
Like this they have no title, so they just appear as an arrow.
One can define a Heading Text like so: `+ heading text +-- [...] --+`.
Every one-line Element can be used as heading text.

The second symbol in the opening sequence defines its default state: `+` = open, `-` = closed

Input:
```md
+ This heading is collapsible +--

And here is a definition what happens when you do things
--+

++- This text is quite the collapsible kind. --+
```

Output:
```html
<details>
	<summary><h1>This heading is collapsible</h1></summary>
	<p>
	And here is a definition what happens when you do things
	</p>
</details>

<details open>
<summary></summary>
This text is quite the collapsible kind.
</details>
```

If you want to wrap every Heading in a collapsible block, you can do this with the short-syntax:

```md
+## Heading 2, collapsible

Text'n'stuff

-## Heading 2, collapsible

+### Heading 3, open

howdy

```

```html
<details open>
<summary><h2>Heading 2, collapsible</h2></summary>
<p>
Text'n'stuff
</p>
</details>

<details>
<summary><h2>Heading 2, collapsible</h2></summary>

	<details open>
	<summary><h2>Heading 3, open</h2></summary>
	<p>
		howdy
	</p>
	</details>
</details>
```

### Replace Content

If it is cumbersome to read long link descriptions in plain text, one can define variables with `%`, followed by a word (containing no spaces). Variable names have to be all-lowercase and contain no spaces. In that regard they are trated like heading ids. If no name is given, the content is used (Links only).

To Insert Content into any element one can use sharp brackets `<%name>`, optionally with content to show if the name definition is not found `[Not found]<%name>`

They dont work in plain Text though. They only work in combination with Links of any kind or Commands.

Input:
```md
I dont want to link to [Wikipedia](%wiki) right now.

But fast things like [Web Search](%) are okay.

This sentence is finished [%3]

Imagine content that will be defined [in the future]{%1}

%(wiki): wikipedia.org :"The knowledge database of humanity"
%(web-search): google.com
%{1}: >color:red
%<name>
```

Output:
```html
I dont want to link to <a href="wikipedia.org" 
title="The knowledge database of humanity">Wikipedia</a> right now.

<p>
Imagine content that will be defined <span style="color:red">in the future</span>
</p>
```

### Types of Links

```md
[URL Links](google.com :"With optional Description")
[Images]!(/img/cat.png :Cute)
[Footnotes]^("optional text" :"And description")
[Headings]#(:"With description")
[Replace-Content]<%name>
[Commands]{}
```
