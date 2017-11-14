---
title:  A custom binary exporter in blender
date:   November, 2017
author: Morten H. Solvang
---

I use blender's built in python scripting capabilities to export models in a custom binary format. This allows me to *"draw"* hitboxes in blender and use them directly in my game. 

In this post I will talk a bit about how I wrote the exporter, and which benefits I see in using it over using blender's default exporters. Note that there won't be a complete code example, as you probably want to write a custom format yourself if you are doing this.

![Figure 1: The process, exemplified by beautifull programmer art](figures/custom_blender_export_figure_1.png)

For reference, this was done in blender version *2.75*. Things might be different in other versions!

## Why?

Writing the exporter and corresponding importer definetly was a bit of work, but I still think that it was a worthwhile project. For one, I really enjoyed working on it, and usually that in itself is enough justification when working on a hobby project. There are however some tangible benefits over other ways of exporting:

For one, I can retrieve exactly the data I am interested in. With other model formats, you have to sift through the information stored in the format to get the values you want in your game. With a custom exporter, you can move this sifting to the exporter as you have full control over the entire pipeline. Additionally, I know for certain that I can parse every valid model file my generator outputs.

As shown in figure 1, I can bundle additional data in my model exports. I am sure you could somehow hoax formats like `.fbx` or `.dae` into doing something like this, but the complexity of these formats have never alowed me to use them efficiently. Being able to place hitboxes directly inside blender gives a nice usability and productivity benefit, as I don't have to fiddle around with some other custom solution.



## The basics

First, you need to set yourself up for python scripting in blender. There are two "tools" to use: The built-in console and the text editor. Figure 2 shows a sample setup.

![Figure 2: The basic setup for scripting in blender](figures/custom_blender_export_figure_2.png)

Legend for figure 2:

1. Output in the system console. `print()` prints to the system console, which you can toggle via `Window > Toggle System Console`

2. You switch to the text editor using this dropdown. You can also write in an external editor, which is a lot nicer. See [here](https://blender.stackexchange.com/a/56709) for instructions on how to run an external file (Not sure if the solution there is *good* though).

3. Press this button to run your script. For me, this exports all the models

4. The interactive console can be found in the same dropdown menu as the text editor. It has autocomplete (`Ctrl + Space`), which coupled with some guesswork gets you a long way in finding the which fields an object has.

5. You can have multiple files open in blender's text editor, and this menu switches between them. At first, it will say `+ New`. If you press it once, it creates a new "file". You have to do this before you can start writing code.

Lastly, you should `import bpy` to access blender's python api.


## Getting data from blender

I'll briefly cover how you can get model data from blender. For more detailed info, you can refer to [blender's api](https://docs.blender.org/api/current/).

All objects in the scene can be iterated over with `for object in bpy.context.scene.objects`. You probably only want to export meshes, which you can check for with `object.type == 'MESH'`.

When exporting mesh data, you probably want to triangulate the mesh first, as thsi is the format you want it in when rendering. For objects which are meshes, this can be done as follows:

```python
import bmesh

# Create a mesh you can manipulate from the object data
mesh = bmesh.new()
mesh.from_mesh(object.data)

# Split quads etc. into triangles
bmesh.ops.triangulate(mesh, faces=mesh.faces[:])

# Iterate over faces and vertices.
for face in mesh.faces:
    material_index = face.material_index
    normal = face.normal # for flat shading

    for vert in face.verts:
        position = vert.co
        normal = vert.normal # for smooth shading

mesh.free()
```


You probably also want to export materials, which can be retrieved as follows:

```python
for material_slot in object.material_slots:
    material = material_slot.material
    if material is None:
        # You probably want to cancel the export here
    else:
        diffuse = material.diffuse_color
```

For the hitboxes themselves, I don't actually use meshes. Instead, I only create cubes and modify their position, rotation and scaling. These values can be retrieved through `object.location`, `object.rotation_*` and `object.dimensions`. Blender stores rotation in different variables depending on the rotation mode selected in the editor. By default, it uses `object.rotation_euler`. You can call `.to_quaternion()` on this if you want to export quaternions. Note that these values only are "reliable" if you don't modify the mesh of an object! 


## Writing a binary file in python

The last step is actually exporting the data. I chose a binary format, as it seemed simpler both to write and parse. Note that I won't be covering the parsing of the format here, as that is somewhat language dependent.

In python, you can write binary data to a file using `struct.pack` (remember to `import struct` first). For example, `struct.pack('<H', my_variable)` creates a binary string of an unsigned 16 bit integer in little endian. Further documentation can be found at [``](https://docs.python.org/3/library/struct.html).

You can open a file for writing with `open(file_path, 'wb')`. `'wb'` stands for write binary.

A simple example:
```python
filepath = ""
with open(filepath, 'wb') as file:
    my_vector = ...
    file.write("<fff", my_vector.x, my_vector.y, my_vector.z)
```

In my exporter, I write a long sequence of vectors to represent a mesh. When importing, I then need to know how long this sequence is so I can read it back from the file. Because of this, I write out the number of elements in a sequence before I write the sequence itself. The same goes for writing strings.

Regarding strings, you also have to encode them into a binary string before writing them. This goes as follows:

```python
text = object.name
encoded_text = text.encode('utf-8')

file.write(struct.pack("<H", len(encoded_text)))
file.write(encoded_text)
```


## Conclusion

Writing this exporter has allowed me to better integrate blender in my game making process. Furthermore, it brings me a certain joy having finally found what I think is a relatively clean solution to the problem of model importing and exporting. The files created by my exporter are about the same size as standard obj files, but they could probably be made a lot smaller if you were to put in the effort.
