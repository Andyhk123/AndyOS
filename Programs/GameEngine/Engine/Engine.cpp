#include "GEngine.h"
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>

namespace GEngine {
unsigned int ticks;
int totalFrames = 0;
int startTicks = 0;

bool BOOL1 = false;

GC gc;
bool paused = false;
bool raytrace = false;

float deltaTime;
Game *game;
gui::Window *window;

Vector3 WorldToScreen(Vector3 &point)
{
    Matrix4 P = Matrix4::CreatePerspectiveProjection(gc.width, gc.height, 90, 1, 10);

    Camera *cam = game->GetActiveCamera();
    Matrix4 V = Matrix4::CreateView(
        -cam->transform.GetForwardVector().ToVector4(), cam->transform.GetUpVector().ToVector4(),
        cam->transform.GetRightVector().ToVector4(), cam->transform.position.ToVector4());

    Vector4 p = P * V * point.ToVector4();

    if (p.w <= 0)
        return Vector3(0, 0, 0);

    Vector3 screen;
    screen.x = p.x * gc.width / p.w + gc.width * 0.5;
    screen.y = p.y * gc.height / p.w + gc.height * 0.5;

    // if (isnan(screen.x) || isnan(screen.y))
    //{
    //	printf("%f\t%f\t%f\n", p.x, p.y, p.w);
    //}

    return screen;
}

void DebugLine(Vector3 start, Vector3 end, Color &color)
{
    Vector3 lpstart = WorldToScreen(start);
    Vector3 lpend = WorldToScreen(end);

    float total = lpstart.x + lpend.x + lpstart.y + lpend.y;
    if (isnan(total)) {
        return;
        printf("%f\t%f\t%i\t%i\t%i\n", lpstart.x, lpstart.y, (int)start.x, (int)start.y,
               (int)start.z);
        printf("%f\t%f\t%i\t%i\t%i\n", lpstart.y, lpstart.y, (int)end.x, (int)end.y, (int)end.z);
        while (1)
            ;
    }

    gc.DrawLine(lpstart.x, lpstart.y, lpend.x, lpend.y, color);
}

void DebugBox(Box &box, Color &color)
{
    Vector3 maxx = Vector3(box.max.x, box.min.y, box.min.z);
    Vector3 maxy = Vector3(box.min.x, box.max.y, box.min.z);
    Vector3 maxz = Vector3(box.min.x, box.min.y, box.max.z);

    Vector3 minx = Vector3(box.min.x, box.max.y, box.max.z);
    Vector3 miny = Vector3(box.max.x, box.min.y, box.max.z);
    Vector3 minz = Vector3(box.max.x, box.max.y, box.min.z);

    DebugLine(box.min, maxx, color);
    DebugLine(box.min, maxy, color);
    DebugLine(box.min, maxz, color);

    DebugLine(box.max, minx, color);
    DebugLine(box.max, miny, color);
    DebugLine(box.max, minz, color);

    DebugLine(maxx, miny, color);
    DebugLine(maxy, minz, color);
    DebugLine(maxz, minx, color);

    DebugLine(minx, maxy, color);
    DebugLine(miny, maxz, color);
    DebugLine(minz, maxx, color);
}

void PrintMatrix(Matrix3 M)
{
    for (int i = 0; i < 3; i++) {
        printf("[");
        for (int j = 0; j < 3; j++) {
            printf("%f", M[i * 3 + j]);

            if (j < 2)
                printf("\t");
        }
        printf("]\n");
    }
}

void PrintMatrix4(Matrix4 M)
{
    for (int i = 0; i < 4; i++) {
        printf("[");
        for (int j = 0; j < 3; j++) {
            printf("%f", M[i * 4 + j]);

            if (j < 3)
                printf("\t");
        }
        printf("]\n");
    }
}

Vector3 FrictionForce(Rigidbody *body, Vector3 normal, Vector3 vrel, Vector3 netForce)
{
    float fcs = 0.3;
    float fcd = 0.3;

    Vector3 normalForce = normal * netForce.Dot(normal);

    Vector3 Fs = normalForce * fcs;
    Vector3 Fk = normalForce * fcd;

    Vector3 t;

    if (fabs(vrel.Dot(normal)) < 0.1) {
        if (fabs(netForce.Dot(normal)) > 0.1) {
            // t = (netForce - (normal * netForce.Dot(normal))).Normalized();
        }
    } else {
        // t = (vrel - (vrel * max(0.f, netForce.Normalized().Dot(normal)))).Normalized();
        Vector3 nv = vrel.Normalized();
        t = (nv - (normal * nv.Dot(normal))).Normalized();
    }

    return -netForce / 4;

    Vector3 Ff = t * netForce.Dot(t);

    // if (Ff.Magnitude() > Fs.Magnitude())
    //	return -Ff;

    return normalForce - netForce;
    // return t * Fk.Magnitude();
    return Vector3();
    // if (fd < 10)
    //	return -(netForce - normal * netForce.Dot(normal));
    // else

    Vector3 fo = (netForce - (netForce * netForce.Dot(normal))).Normalized();
    return fo;

    return t * netForce.Dot(vrel.Normalized());
}

void Start()
{
    PlayerManager::CreatePlayer(1, "Player1");
    game->Init();

    for (int i = 0; i < game->objects.size(); i++) {
        GameObject *obj = game->objects[i];
        obj->Start();
    }

    for (int i = 0; i < game->objects.size(); i++) {
        GameObject *obj = game->objects[i];

        for (int j = 0; j < obj->components.size(); j++) {
            Component *comp = obj->components[j];
            comp->Start();
        }
    }
}

void Update()
{
    deltaTime = (get_ticks() - ticks) / 1000000.f;
    ticks = get_ticks();

    for (int i = 0; i < game->objects.size(); i++) {
        GameObject *obj = game->objects[i];
        PlayerManager::SetPlayer(obj->GetOwner());
        obj->Update(deltaTime);
    }

    for (int i = 0; i < game->objects.size(); i++) {
        GameObject *obj = game->objects[i];

        for (int j = 0; j < obj->components.size(); j++) {
            Component *comp = obj->components[j];
            PlayerManager::SetPlayer(comp->GetOwner());
            comp->Update(deltaTime);
        }
    }

    for (int i = 0; i < game->objects.size(); i++) {
        GameObject *obj = game->objects[i];
        PlayerManager::SetPlayer(obj->GetOwner());
        obj->LateUpdate(deltaTime);
    }

    for (int i = 0; i < game->objects.size(); i++) {
        GameObject *obj = game->objects[i];

        for (int j = 0; j < obj->components.size(); j++) {
            Component *comp = obj->components[j];
            PlayerManager::SetPlayer(comp->GetOwner());
            comp->LateUpdate(deltaTime);
        }
    }
}

void Collision()
{
    float energy = 0;

    std::vector<Rigidbody *> all;

    for (int i = 0; i < game->objects.size(); i++) {
        GameObject *obj = game->objects[i];

        Rigidbody *comp = obj->rigidbody;

        if (comp && comp->bEnabled) {
            all.push_back(comp);

            if (comp->bEnabledGravity)
                energy += 1 * 9.8 * (obj->GetWorldPosition().y + 1000);

            energy += 0.5 * comp->SpeedSquared();
            energy += 0.5 * comp->angularVelocity.MagnitudeSquared();
        }
    }

    // printf("Energy: %f\n", energy);

    for (int i = 0; i < all.size(); i++) {
        int start = i + 1;
        for (int j = start; j < all.size(); j++) {
            // if (all[i]->IsBox() && all[j]->IsBox())
            if (1) {
                Rigidbody *a = (Rigidbody *)all[i];
                Rigidbody *b = (Rigidbody *)all[j];
                // a->collider->size = a->parent->GetWorldScale();
                // b->collider->size = b->parent->GetWorldScale();

                Vector3 mtv;
                int count;
                Manifold *man;

                ::Collision test;

                if (test.TestIntersection(*a, *b, &mtv, man, count)) {
                    a->velocity = Vector3(0, 0, 0);
                    b->velocity = Vector3(0, 0, 0);

                    // a->parent->transform.position += mtv * b->mass / (a->mass + b->mass);
                    // b->parent->transform.position -= mtv * a->mass / (a->mass + b->mass);

                    continue;

                    // DebugLine(a->parent->GetWorldPosition(), a->parent->GetWorldPosition() - mtv,
                    // Color::Blue);

                    if (count == 0)
                        continue;

                    Vector3 colPoint;

                    for (int p = 0; p < count; p++) {
                        Manifold &m = man[p];
                        colPoint += m.Point / count;

                        Vector3 sc = WorldToScreen(m.Point);
                        gc.FillRect(sc.x - 5, sc.y - 5, 10, 10, Color::Red);
                    }

                    Vector3 sc = WorldToScreen(colPoint);
                    gc.FillRect(sc.x - 5, sc.y - 5, 10, 10, Color::Blue);

                    Quaternion rota = a->parent->GetWorldRotation();
                    Quaternion rotb = b->parent->GetWorldRotation();

                    Vector3 posa = a->parent->GetWorldPosition();
                    Vector3 posb = b->parent->GetWorldPosition();

                    Vector3 scalea = a->parent->GetWorldScale();
                    Vector3 scaleb = b->parent->GetWorldScale();

                    Vector3 dp = posa - posb;

                    Vector3 va = a->velocity;
                    Vector3 vb = b->velocity;

                    Vector3 dv = va - vb;
                    Vector3 dir = dp.Normalized();
                    Vector3 dirv = dv.Normalized();
                    float dot = dir.Dot(dirv);

                    // if (dot < 0 && !BOOL1)
                    {
                        // BOOL1 = 1;

                        // a->mass = 1;
                        // b->mass = 1;

                        float e = 0.2;
                        float ma = a->mass;
                        float mb = b->mass;

                        Matrix3 IbodyA = Matrix3();
                        Matrix3 IbodyB = Matrix3();

                        Vector3 sizea =
                            Vector3(a->collider->size.x * scalea.x, a->collider->size.y * scalea.y,
                                    a->collider->size.z * scalea.z);
                        IbodyA[0] = ma * (sizea.y * sizea.y + sizea.z * sizea.z) / 12;
                        IbodyA[4] = ma * (sizea.x * sizea.x + sizea.z * sizea.z) / 12;
                        IbodyA[8] = ma * (sizea.x * sizea.x + sizea.y * sizea.y) / 12;

                        Vector3 sizeb =
                            Vector3(b->collider->size.x * scaleb.x, b->collider->size.y * scaleb.y,
                                    b->collider->size.z * scaleb.z);
                        IbodyB[0] = mb * (sizeb.y * sizeb.y + sizeb.z * sizeb.z) / 12;
                        IbodyB[4] = mb * (sizeb.x * sizeb.x + sizeb.z * sizeb.z) / 12;
                        IbodyB[8] = mb * (sizeb.x * sizeb.x + sizeb.y * sizeb.y) / 12;

                        Matrix3 MRA = rota.ToMatrix3();
                        Matrix3 MRB = rotb.ToMatrix3();
                        // Matrix3 MatR =
                        // Matrix3::CreateRotation(a->parent->transform.rotation.ToEuler());
                        // printf("%f %f %f %f %f %f", Ia[0], Ia[1], Ia[2], Ia[3], Ia[4], Ia[5]);
                        // Matrix3 MatR2 =
                        // Matrix3::CreateRotation(b->parent->transform.rotation.ToEuler());

                        Matrix3 invA = (MRA * IbodyA.Inverse() * MRA.Transpose());
                        Matrix3 invB = (MRB * IbodyB.Inverse() * MRB.Transpose());

                        Vector3 Xa = posa;
                        Vector3 Xb = posb;

                        Vector3 n = mtv.Normalized();

                        // colPoint = Vector3(-1, 0.5, -1);

                        // DrawLine(colPoint, colPoint + n, 0xFF0000FF);
                        Vector3 ra = /*-rota * */ (colPoint - Xa);
                        Vector3 rb = /*-rotb * */ (colPoint - Xb);

                        Vector3 pa = a->velocity + a->angularVelocity.Cross(ra);
                        Vector3 pb = b->velocity + b->angularVelocity.Cross(rb);
                        float Vrel = n.Dot(pb - pa);

                        float N = Vrel * -(1 + e);
                        float t1 = 1 / ma;
                        float t2 = 1 / mb;
                        float t3 = ra.Cross(n).Dot(invA * ra.Cross(n));
                        float t4 = rb.Cross(n).Dot(invB * rb.Cross(n));

                        // t3 = n.Dot(inva * ra.Cross(n).Cross(ra));
                        // t4 = n.Dot(invb * rb.Cross(n).Cross(rb));

                        // t3 = n.Dot((inva * ra.Cross(n)).Cross(ra));
                        // t4 = n.Dot((invb * rb.Cross(n)).Cross(rb));

                        // float t3 = 0;
                        // float t4 = n.Dot((inva * ra.Cross(n)).Cross(ra) + (invb *
                        // rb.Cross(n)).Cross(rb));

                        float j = N / (t1 + t2 + t3 + t4);
                        Vector3 force = n * j;

                        // sleep(10000);

                        printf("N %f\n", N);
                        printf("%f\t%f\t%f\t%f\n", t1, t2, t3, t4);
                        printf("Acc %f\n", force.y / a->mass);
                        // PIT::Sleep(100000);
                        // PIT::Sleep(100);

                        if (force.Dot(n) < 0) {
                            a->velocity -= force / ma;
                            b->velocity += force / mb;

                            // Vector3 t = force.Cross(ra);
                            DebugLine(colPoint, colPoint + force * 10, Color::Blue);
                            a->angularVelocity += (invA * ra.Cross(-force));

                            DebugLine(Xa, Xa + (invA * ra.Cross(force)), Color::Cyan);
                            // b->angularVelocity += (invB * rb.Cross(force));

                            /*a->AddImpulse(-force);
                            b->AddImpulse(force);

                            ra = -rota * (colPoint - Xa);
                            rb = -rotb * (colPoint - Xb);

                            pa = (a->velocity + a->angularVelocity.Cross(ra));
                            pb = (b->velocity + b->angularVelocity.Cross(rb));
                            Vrel = n.Dot(pb - pa);

                            N = Vrel * -(1 + e);
                            t1 = 1 / ma;
                            t2 = 1 / mb;
                            t3 = ra.Cross(n).Dot(invA * ra.Cross(n));
                            t4 = rb.Cross(n).Dot(invB * rb.Cross(n));

                            J = N / (t1 + t2 + t3 + t4);
                            force = n * J;

                            Vector3 waf = invA * Vector3::Cross(ra, force);
                            Vector3 wbf = invB * Vector3::Cross(rb, force);

                            a->angularVelocity -= waf;
                            b->angularVelocity += wbf;

                            DebugLine(colPoint, colPoint + waf * sqrt(waf.Magnitude()) * 10,
                            Color::Green);

                            //a->angularVelocity.x = 0;
                            //a->angularVelocity.y = 0;
                            //
                            //b->angularVelocity.x = 0;
                            //b->angularVelocity.y = 0;

                            DebugLine(colPoint, colPoint + force * sqrt(force.Magnitude()) * 10,
                            Color::Red);
                            //PIT::Sleep(1000);


                            //DrawLine(colPoint, colPoint + t * 10, 0xFFFF00FF);*/

                            /*continue;
                            Vector3 ffa = FrictionForce(a, -n, vr, (a->bEnabledGravity ? Vector3(0,
                            -9.8, 0) * a->mass : Vector3())); Vector3 ffb = FrictionForce(b, n, vr,
                            (b->bEnabledGravity ? Vector3(0, -9.8, 0) * b->mass : Vector3()));

                            DebugLine(colPoint, colPoint + ffa * 10, Color::Yellow);

                            a->AddImpulse(ffa * deltaTime);
                            b->AddImpulse(ffb * deltaTime);
                            usleep(10000);*/
                        }
                    }
                }
            }
            // else if (all[i]->IsSphere() && all[j]->IsSphere())
            else {
                Rigidbody *a = all[i];
                Rigidbody *b = all[j];

                Vector3 posa = a->parent->GetWorldPosition() + a->parent->transform.position;
                Vector3 posb = b->parent->GetWorldPosition() + b->parent->transform.position;

                Vector3 dp = posa - posb;

                float d = dp.Magnitude() / (2 + 2);
                if (d < 1) {
                    // a->parent->Translate(Vector3(0, 10, 0));

                    Vector3 va;
                    Vector3 vb;

                    va = a->velocity;
                    vb = b->velocity;

                    Vector3 dv = va - vb;
                    Vector3 dir = dp.Normalized();
                    Vector3 dirv = dv.Normalized();
                    float dot = dir.Dot(dirv);

                    if (dot < 0) {
                        Vector3 imp = dir * dot * dv.Magnitude() / 1.5;
                        a->AddImpulse(-imp);
                        b->AddImpulse(imp);
                    }
                }
            }
        }
    }
}

void PostRender(GC &gc)
{
    char buf[128];

    if (totalFrames > 0) {
        snprintf(buf, sizeof(buf), "DT:%i  AvgDT: %.1f  Time: %.1f", (get_ticks() - ticks) / 1000,
                 ((ticks - startTicks) / totalFrames) / 1000.f, ticks / 1000000.f);

        gc.DrawText(0, 0, buf, Color::Red);

        snprintf(buf, sizeof(buf), "FPS: %.0f", 1000000.f / (get_ticks() - ticks));
        gc.DrawText(0, 16, buf, Color::Red);

        snprintf(buf, sizeof(buf), "%dx%d", gc.width, gc.height);
        gc.DrawText(0, 32, buf, Color::Red);

        Camera *cam = game->GetActiveCamera();
        Vector3 pos = cam->transform.position;
        Vector3 euler = cam->transform.rotation.ToEuler();
        snprintf(buf, sizeof(buf), "Camera: [%.1f, %.1f, %.1f] [%.1f, %.1f, %.1f]", pos.x, pos.y,
                 pos.z, euler.x, euler.y, euler.z);
        gc.DrawText(0, 48, buf, Color::Red);
    }
}

void Render()
{
    if (raytrace) {
        Raytracer tracer(gc);
        tracer.Render();
        return;
    }

    Camera *cam = game->GetActiveCamera();
    Matrix4 V = Matrix4::CreateView(
        -cam->transform.GetForwardVector().ToVector4(), cam->transform.GetUpVector().ToVector4(),
        cam->transform.GetRightVector().ToVector4(), cam->transform.position.ToVector4());

    Vector3 ang = cam->GetWorldRotation() * Vector3(0, 0, 1);
    GL::CameraDirection(ang.ToVector4());

    GL::Clear(0x7F7F7F);
    GL::MatrixMode(GL_VIEW);
    GL::LoadMatrix(V);
    GL::MatrixMode(GL_MODEL);

    for (int i = 0; i < game->objects.size(); i++) {
        GameObject *obj = game->objects[i];

        for (int j = 0; j < obj->meshComponents.size(); j++) {
            MeshComponent *mesh = obj->meshComponents[j];

            if (!mesh)
                continue;

            Model3D *model = mesh->model;

            if (!model)
                continue;

            GameObject *parent = mesh->parent;
            Transform trans = parent->GetWorldTransform();
            Matrix4 T = Matrix4::CreateTranslation(trans.position.ToVector4());
            Matrix4 R = trans.rotation.ToMatrix();
            Matrix4 S = Matrix4::CreateScale(trans.scale.ToVector4());

            for (int v = 0; v < mesh->model->vertices.size(); v++)
                model->vertex_buffer[v].world_normal = R * model->vertex_buffer[v].normal;

            Matrix4 M = T * R * S;

            GL::LoadMatrix(M);

            GL::BindTexture(mesh->texId);
            GL::Draw(model->vertex_buffer, model->vertices.size());
        }
    }

    PostRender(GL::GetGC());
    GL::SwapBuffers();
}

void StartGame(Game *game, gui::Window *wnd)
{
    GEngine::game = game;
    GEngine::window = wnd;

    deltaTime = 1 / 100.0f;
    startTicks = get_ticks();
    ticks = startTicks;

    Input::Init();

    gc = wnd->gc;
    GL::InitGraphics(gc);

    Start();

    GL::MatrixMode(GL_PROJECTION);
    GL::LoadMatrix(Matrix4::CreatePerspectiveProjection(gc.width, gc.height, 90, 1, 10));

    while (true) {
        PlayerManager::SetPlayer(0);
        Input::Update(wnd->active);

        if (Input::GetKeyDown(KEY_ESCAPE)) {
            paused = !paused;
            window->SetCapture(paused);
        }

        if (Input::GetKeyDown(KEY_P))
            raytrace = !raytrace;

        if (window->gc.width != gc.width || window->gc.height != gc.height) {
            gc = window->gc;
            GL::InitGraphics(gc);
            GL::MatrixMode(GL_PROJECTION);
            GL::LoadMatrix(Matrix4::CreatePerspectiveProjection(gc.width, gc.height, 90, 1, 10));
        }

        game->GetNetworkManager()->ProcessPackets();
        game->GetNetworkManager()->SendPackets();

        Update();
        Collision();
        Render();

        window->Paint();

        totalFrames++;
    }
}
} // namespace GEngine
