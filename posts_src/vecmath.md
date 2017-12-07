---
title:  2d rotation using complex numbers
date:   December, 2017
author: Morten H. Solvang
---

In this post, I will go over how we can use some of the properties of complex numbers to rotate 2d vectors. Instead of just going over the raw mathemathics, I have created a simple interactive demo of complex number, which hopefully will give you a good intuition of how complex numbers work:

<!-- We manually insert figure 1 because it is a canvas -->
<figure>
<canvas id="figure1" width=424 height=393>
<img src="../figures/vecmath_figure_1_fast.svg"></img>
</canvas>
<figcaption>Figure 1: Try moving $p$ and $q$ around, and see how $w$ reacts!</figcaption>
</figure>
<script src="../other/vecmath_script.js"></script>

Feel free to spend a moment playing with figure 1.

At first, it might seem like there is not much of a pattern to how $w$ reacts when you move $p$ and $q$. Lets look into what happens behind the scenes:

$p$, $q$ and $w$ are all complex numbers. That is, they are a sum of some real number $x$ and some imaginary number $y \cdot i$. $x$ is placed along the real axis, which is horizontal, while $y$ is placed along the imaginary axis, which is vertical. Below are some examples. Press the buttons to move the points in figure 1.

<table>
    <tr><td>$p = 0.5$</td><td><button id="example_button_1">Show</button></td></tr>
    <tr><td>$p = -0.75i$</td><td><button id="example_button_2">Show</button></td></tr>
    <tr><td>$p = -0.5 + i$</td><td><button id="example_button_3">Show</button></td></tr>
</table>

## Polar and cartesian form

A complex number $z$ is the sum of some real number $a$ and some imaginary number $b \cdot i$. (Remember, $i = \sqrt{-1}$). $z$ is then:
$$z = a + b \cdot i$$
If we try to place $z$ on the number line, we run into an issue: $i$ is nowhere to be found. Instead, we have to place $b \cdot i$ along a separate number line. This gives us a pair of number lines, a number plane, as shown in figure 2a.

Complex numbers are also commonly represented in polar form. That is, we have some angle $\varphi$ and some radius $r$, giving us a point on the complex plane. See figure 2b for an example of this. We can convert the pair $(\varphi, r)$ to cartesian form using trigonometry:

$$a = r \cdot cos \varphi$$
$$b = r \cdot sin \varphi$$

![Figure 2: $z = 2 + 3i$ on the complex number plane, shown in __(a)__ cartesian form and __(b)__ polar form](../figures/vecmath_figure_2_fast.svg)

To summarize: We can represent a complex number either as a pair or real numbers $(a, b)$, or as a angle coupled with a real number $(\varphi, r)$. Note that both forms can represent the same number.

## Multiplying complex numbers

Multiplying two complex numbers in cartesian form is relatively straight forward, though it might look a bit messy. The one thing you have to remember is that $i^2 = (\sqrt{-1})^2 = -1$. Lets multiply the complex numbers $z_1 = a + b \cdot i$ and $z_2 = c + d \cdot i$:

$$z_1 \cdot z_2 = (a + b \cdot i) \cdot (c + d \cdot i) = $$
$$ac + bc \cdot i + ad \cdot i + bc \cdot i^2 =$$ 
$$(ac - bd) + (ad + bc) \cdot i$$

ROTATION STUFF PLEASE

## Going back to vectors

One thought that might have struck you while looking at figure 2a, is that it looks just like a 2d coordinate system. It turns out that we, for all intents and purposes, can treat complex numbers like 2d vectors. A 2d vector $(x, y)$ then corresponds to the complex number $x + y\cdot i$.

This means we can apply complex multiplication to vectors:

$$ (x_a, y_a) \otimes (x_b, y_b) = (x_a x_b - y_a y_b, x_a y_b - x_b y_a) $$

In code, this would look something like the following

```rust
complex_multiply(Vector2 a, Vector2 b) -> c {
    x = a.x*b.x - a.y*b.y;
    y = a.x*b.y + a.y*b.x;
    return Vector2 { x, y };
}
```
