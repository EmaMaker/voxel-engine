#include <array>
#include <memory>

#include "block.hpp"
#include "chunk.hpp"
#include "chunkmesh.hpp"
#include "globals.hpp"
#include "spacefilling.hpp"
#include "utils.hpp"

ChunkMesh::~ChunkMesh()
{
    delete this->chunk;
    delete (this->theShader);
}

ChunkMesh::ChunkMesh(Chunk::Chunk *c)
{
    this->chunk = c;

    glGenVertexArrays(1, &(this->VAO));
    glGenBuffers(1, &(this->colorBuffer));
    glGenBuffers(1, &(this->VBO));
    glGenBuffers(1, &(this->EBO));

    this->theShader = new Shader{"shaders/shader.vs", "shaders/shader.fs"};
    this->model = glm::translate(model, (float)CHUNK_SIZE * c->getPosition());

    // std::cout << "CHUNK MESH " << c << std::endl;
}

void ChunkMesh::mesh()
{

    /*
     * Taking inspiration from 0fps and the jme3 porting at
     * https://github.com/roboleary/GreedyMesh/blob/master/src/mygame/Main.java
     *
     * By carefully re-reading the code and the blog post, I've come to the
     * realization that I wrote something very similar yet a lot messier (and
     * uglier) back in
     * https://github.com/EmaMaker/voxel-engine-jme3/blob/master/src/voxelengine
     * /world/Chunk.java
     *
     * Reading roboleary's impl. I've learned how to optimize having to loop
     * across different planes everytime I change dimension without having to
     * write 3 separate 3-nested-for-loops
     */
    /*
     * It's not feasible to just create a new mesh everytime a quad needs placing.
     * This ends up creating TONS of meshes and the game will just lag out.
     * As I did in the past, it's better to create a single mesh for each chunk,
     * containing all the quads. In the future, maybe translucent blocks and liquids
     * will need a separate mesh, but still on a per-chunk basis
     */
    vertices.clear();
    indices.clear();
    vIndex = 0;

    // convert tree to array since it is easier to work with it
    int length{0};
    Block *blocks = this->chunk->getBlocksArray(&length);

    if (length == 0)
        return;

    int k, l, u, v, w, h, n, j, i;
    int x[]{0, 0, 0};
    int q[]{0, 0, 0};
    int du[]{0, 0, 0};
    int dv[]{0, 0, 0};

    std::array<Block, CHUNK_SIZE * CHUNK_SIZE> mask;
    for (bool backFace = true, b = false; b != backFace; backFace = backFace && b, b = !b)
    {
        // iterate over 3 dimensions
        for (int dim = 0; dim < 3; dim++)
        {
            // offsets of other 2 axes
            u = (dim + 1) % 3;
            v = (dim + 2) % 3;

            x[0] = 0;
            x[1] = 0;
            x[2] = 0;

            q[0] = 0;
            q[1] = 0;
            q[2] = 0;
            q[dim] = 1; // easily mark which dimension we are comparing
                        // voxels
                        // on

            for (x[dim] = -1; x[dim] < CHUNK_SIZE;)
            {
                n = 0;

                for (x[v] = 0; x[v] < CHUNK_SIZE; x[v]++)
                {
                    for (x[u] = 0; x[u] < CHUNK_SIZE; x[u]++)
                    {
                        Block b1 = (x[dim] >= 0) ? blocks[HILBERT_XYZ_ENCODE[x[0]][x[1]][x[2]]]: Block::NULLBLK;
                        Block b2 = (x[dim] < CHUNK_SIZE - 1)
                                       ? blocks[HILBERT_XYZ_ENCODE[x[0] + q[0]][x[1] + q[1]][x[2] + q[2]]]
                                       : Block::NULLBLK; 

                        // This is the original line taken from rob's code, readapted (replace voxelFace
                        // with b1 and b2).
                        // mask[n++] = ((voxelFace != Block::NULLBLK && voxelFace1 != Block::NULLBLK &&
                        // voxelFace.equals(voxelFace1))) ? Block::NULLBLK : backFace ? voxelFace1 : voxelFace;

                        // Additionally checking whether b1 and b2 are AIR or Block::NULLBLK allows face culling,
                        // thus not rendering faces that cannot be seen
                        // Removing the control for Block::NULLBLK disables chunk borders
                        // This can be surely refactored in something that isn't such a big one-liner
                        mask[n++] = b1 != Block::NULLBLK && b2 != Block::NULLBLK && b1 == b2 ? Block::NULLBLK
                                    : backFace                                               ? b1 == Block::AIR || b1 == Block::NULLBLK ? b2 : Block::NULLBLK
                                    : b2 == Block::AIR || b2 == Block::NULLBLK               ? b1
                                                                                             : Block::NULLBLK;
                    }
                }

                x[dim]++;
                n = 0;
                // Actually generate the mesh from the mask. This is the same thing I used in my old crappy voxel engine
                for (j = 0; j < CHUNK_SIZE; j++)
                {
                    for (i = 0; i < CHUNK_SIZE;)
                    {

                        if (mask[n] != Block::NULLBLK)
                        {
                            // First compute the width
                            for (w = 1; i + w < CHUNK_SIZE && mask[n + w] != Block::NULLBLK && mask[n] == mask[n + w]; w++)
                            {
                            }

                            bool done = false;
                            for (h = 1; j + h < CHUNK_SIZE; h++)
                            {
                                for (k = 0; k < w; k++)
                                {
                                    if (mask[n + k + h * CHUNK_SIZE] == Block::NULLBLK || mask[n + k + h * CHUNK_SIZE] != mask[n])
                                    {
                                        done = true;
                                        break;
                                    }
                                }
                                if (done)
                                    break;
                            }

                            if (mask[n] != Block::AIR)
                            {
                                x[u] = i;
                                x[v] = j;

                                du[0] = 0;
                                du[1] = 0;
                                du[2] = 0;
                                du[u] = w;

                                dv[0] = 0;
                                dv[1] = 0;
                                dv[2] = 0;
                                dv[v] = h;

                                quad(glm::vec3(x[0], x[1], x[2]),
                                     glm::vec3(x[0] + du[0], x[1] + du[1], x[2] + du[2]),
                                     glm::vec3(x[0] + du[0] + dv[0], x[1] + du[1] + dv[1],
                                               x[2] + du[2] + dv[2]),
                                     glm::vec3(x[0] + dv[0], x[1] + dv[1], x[2] + dv[2]),
                                     mask[n], backFace);
                            }

                            for (l = 0; l < h; ++l)
                            {
                                for (k = 0; k < w; ++k)
                                {
                                    mask[n + k + l * CHUNK_SIZE] = Block::NULLBLK;
                                }
                            }

                            /*
                             * And then finally increment the counters and
                             * continue
                             */
                            i += w;
                            n += w;
                        }
                        else
                        {
                            i++;
                            n++;
                        }
                    }
                }
            }
        }
    }

    if (blocks)
        delete blocks;

    if (vertices.size() > 0)
    {

        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &(vertices[0]), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &(indices[0]), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat), &(colors[0]), GL_STATIC_DRAW);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

        glBindVertexArray(0);
    }
}

void ChunkMesh::draw()
{

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // wireframe mode
    if (vertices.size() > 0)
    {
        theShader->use();
        theShader->setMat4("model", this->model);
        theShader->setMat4("view", theCamera.getView());
        theShader->setMat4("projection", theCamera.getProjection());

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLuint>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

void ChunkMesh::quad(glm::vec3 bottomLeft, glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomRight, Block block, bool backFace)
{

    vertices.push_back(bottomLeft.x);
    vertices.push_back(bottomLeft.y);
    vertices.push_back(bottomLeft.z);

    vertices.push_back(bottomRight.x);
    vertices.push_back(bottomRight.y);
    vertices.push_back(bottomRight.z);

    vertices.push_back(topLeft.x);
    vertices.push_back(topLeft.y);
    vertices.push_back(topLeft.z);

    vertices.push_back(topRight.x);
    vertices.push_back(topRight.y);
    vertices.push_back(topRight.z);

    indices.push_back(vIndex + 2);
    indices.push_back(vIndex + 3);
    indices.push_back(vIndex + 1);
    indices.push_back(vIndex + 1);
    indices.push_back(vIndex);
    indices.push_back(vIndex + 2);
    vIndex += 4;
    // }
    // else
    // {
    //     indices.push_back(i + 2);
    //     indices.push_back(i + 3);
    //     indices.push_back(i + 1);
    //     indices.push_back(i + 1);
    //     indices.push_back(i);
    //     indices.push_back(i + 2);
    // }

    // ugly switch case
    GLfloat r, g, b;
    switch (block)
    {
    case Block::STONE:
        r = 0.588f;
        g = 0.588f;
        b = 0.588f;
        break;
    case Block::GRASS:
        r = 0.05f;
        g = 0.725f;
        b = 0.0f;
        break;
    case Block::DIRT:
        r = 0.176f;
        g = 0.282f;
        b = 0.169f;
        break;
    default:
        r = 0.0f;
        g = 0.0f;
        b = 0.0f;
        break;
    }

    // Fake shadows
    if ((bottomLeft.z == bottomRight.z && bottomRight.z == topLeft.z && topLeft.z == topRight.z))
    {
        r += 0.1f;
        g += 0.1f;
        b += 0.1f;
    }

    if ((bottomLeft.y == bottomRight.y && bottomRight.y == topLeft.y && topLeft.y == topRight.y))
    {
        r += 0.12f;
        g += 0.12f;
        b += 0.12f;
    }

    for (int i = 0; i < 4; i++)
    {
        colors.push_back(r);
        colors.push_back(g);
        colors.push_back(b);
    }
}