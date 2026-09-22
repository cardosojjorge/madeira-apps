/**
 * Greybox built from chapter location data. Units in the JSON are centimetres.
 */
import * as THREE from "../vendor/three.module.js";

const CM = 0.01;

export function ueToThree(v) {
  return new THREE.Vector3(v.X * CM, v.Z * CM, v.Y * CM);
}

function hexColor(hex) {
  const value = (hex || "12141A").replace("#", "");
  return new THREE.Color(`#${value.length === 6 ? value : "12141A"}`);
}

function boxFromMesh(position, scale) {
  const center = ueToThree(position);
  const size = new THREE.Vector3(Math.max(scale.X, 0.01), Math.max(scale.Z, 0.01), Math.max(scale.Y, 0.01));
  return {
    min: new THREE.Vector3(center.x - size.x / 2, center.y - size.y / 2, center.z - size.z / 2),
    max: new THREE.Vector3(center.x + size.x / 2, center.y + size.y / 2, center.z + size.z / 2),
  };
}

function expandBox(box, minSize) {
  const size = new THREE.Vector3().subVectors(box.max, box.min);
  const center = new THREE.Vector3().addVectors(box.min, box.max).multiplyScalar(0.5);
  size.x = Math.max(size.x, minSize);
  size.y = Math.max(size.y, minSize);
  size.z = Math.max(size.z, minSize);
  return {
    min: new THREE.Vector3(center.x - size.x / 2, center.y - size.y / 2, center.z - size.z / 2),
    max: new THREE.Vector3(center.x + size.x / 2, center.y + size.y / 2, center.z + size.z / 2),
  };
}

function rayBox(origin, dir, box, maxDist) {
  let tmin = 0;
  let tmax = maxDist;
  for (const axis of ["x", "y", "z"]) {
    const o = origin[axis];
    const d = dir[axis];
    if (Math.abs(d) < 1e-8) {
      if (o < box.min[axis] || o > box.max[axis]) return null;
      continue;
    }
    let t1 = (box.min[axis] - o) / d;
    let t2 = (box.max[axis] - o) / d;
    if (t1 > t2) {
      const swap = t1;
      t1 = t2;
      t2 = swap;
    }
    tmin = Math.max(tmin, t1);
    tmax = Math.min(tmax, t2);
    if (tmax < tmin) return null;
  }
  return tmin;
}

function makeMesh(geometry, color, options = {}) {
  const material = new THREE.MeshStandardMaterial({
    color,
    roughness: options.roughness ?? 0.86,
    metalness: options.metalness ?? 0.04,
    emissive: options.emissive || color,
    emissiveIntensity: options.emissiveIntensity ?? 0.12,
  });
  const mesh = new THREE.Mesh(geometry, material);
  mesh.castShadow = true;
  mesh.receiveShadow = true;
  return mesh;
}

export class Greybox {
  constructor(canvas) {
    this.canvas = canvas;
    this.renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
    this.renderer.setClearColor(0x07080c, 1);
    this.renderer.shadowMap.enabled = true;
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 2.45;
    this.scene = new THREE.Scene();
    this.scene.fog = new THREE.FogExp2(0x0c1016, 0.022);
    this.camera = new THREE.PerspectiveCamera(48, 1, 0.05, 80);
    this.group = new THREE.Group();
    this.scene.add(this.group);
    this.solids = [];
    this.interactables = [];
    this.dynamic = new THREE.Group();
    this.group.add(this.dynamic);
    this.player = new THREE.Group();
    this.scene.add(this.player);
    this.yaw = 0;
    this.pitch = 0;
    this.position = new THREE.Vector3(3.8, 0.88, 7);
    this.velocity = new THREE.Vector3();
    this.arm = 2.8;
    this.targetArm = 2.8;
    this.socket = new THREE.Vector3(0, 0.48, 0.62);
    this.focus = null;
    this.saturation = 0.1;
    this.locationId = "";
    this.rain = null;
    this.windowRain = null;
    this.clock = new THREE.Clock();
    this.preset = "High";
    this.buildBody();
    this.buildSkyline();
    this.resize();
  }

  buildBody() {
    const suit = makeMesh(new THREE.BoxGeometry(0.42, 0.9, 0.28), hexColor("14161c"), { emissiveIntensity: 0.02 });
    suit.position.y = 0.15;
    const head = makeMesh(new THREE.SphereGeometry(0.16, 16, 12), hexColor("1c1e22"));
    head.position.y = 0.78;
    this.player.add(suit, head);
  }

  buildSkyline() {
    const city = new THREE.Group();
    const rng = (n) => {
      const x = Math.sin(n * 12.9898) * 43758.5453;
      return x - Math.floor(x);
    };
    for (let i = 0; i < 48; i += 1) {
      const w = 0.8 + rng(i) * 2.4;
      const h = 3 + rng(i + 3) * 14;
      const mesh = makeMesh(new THREE.BoxGeometry(w, h, w), hexColor("0c1016"), { emissiveIntensity: 0.02 });
      const angle = (i / 48) * Math.PI * 2;
      const radius = 28 + rng(i + 8) * 10;
      mesh.position.set(Math.cos(angle) * radius, h / 2 - 1, Math.sin(angle) * radius);
      city.add(mesh);
      if (rng(i + 1) > 0.72) {
        const lamp = new THREE.PointLight(rng(i + 2) > 0.5 ? 0xc4a574 : 0x3d6d78, 1.4, 6);
        lamp.position.copy(mesh.position);
        lamp.position.y += 0.4;
        city.add(lamp);
      }
    }
    this.scene.add(city);
  }

  setPreset(preset) {
    this.preset = preset || "High";
    const density = { Low: 0.012, Medium: 0.018, High: 0.022, Epic: 0.03 }[this.preset] ?? 0.022;
    this.scene.fog.density = density;
    this.renderer.shadowMap.enabled = this.preset !== "Low";
  }

  resize() {
    const width = this.canvas.clientWidth || window.innerWidth;
    const height = this.canvas.clientHeight || window.innerHeight;
    this.renderer.setSize(width, height, false);
    this.camera.aspect = width / Math.max(1, height);
    this.camera.updateProjectionMatrix();
  }

  clearGroup(group) {
    for (const child of [...group.children]) {
      child.traverse?.((obj) => {
        if (obj.geometry) obj.geometry.dispose();
        if (obj.material) {
          if (Array.isArray(obj.material)) obj.material.forEach((m) => m.dispose());
          else obj.material.dispose();
        }
      });
      group.remove(child);
    }
  }

  addSolid(center, scale, color, blocks) {
    const mesh = makeMesh(new THREE.BoxGeometry(1, 1, 1), hexColor(color));
    mesh.position.copy(ueToThree(center));
    mesh.scale.set(scale.x, scale.z, scale.y);
    this.group.add(mesh);
    if (blocks) {
      const half = new THREE.Vector3(scale.x / 2, scale.z / 2, scale.y / 2);
      this.solids.push({
        min: mesh.position.clone().sub(half),
        max: mesh.position.clone().add(half),
      });
    }
    return mesh;
  }

  buildChapter(chapter) {
    this.clearGroup(this.group);
    this.group.add(this.dynamic);
    this.solids = [];
    this.scene.children.filter((obj) => obj.isLight).forEach((light) => this.scene.remove(light));
    const hemi = new THREE.HemisphereLight(0xb7c4d0, 0x1a1e26, 0.9);
    const moon = new THREE.DirectionalLight(0xc5d0dc, 1.15);
    moon.position.set(-8, 12, 4);
    moon.castShadow = this.preset !== "Low";
    this.scene.add(hemi, moon);

    for (const location of chapter.Locations) {
      this.spawnLocation(location);
    }
    this.buildOfficeWindow(chapter);
    this.refreshInteractables(chapter, []);
  }

  spawnLocation(location) {
    const origin = location.Origin;
    const height = location.Height > 0 ? location.Height : 400;
    const floorCenter = { X: origin.X + location.SizeX * 0.5, Y: origin.Y + location.SizeY * 0.5, Z: -10 };
    this.addSolid(floorCenter, { x: location.SizeX * CM, y: location.SizeY * CM, z: 0.2 }, "2A3038", false);
    const ceilCenter = { X: origin.X + location.SizeX * 0.5, Y: origin.Y + location.SizeY * 0.5, Z: height + 8 };
    this.addSolid(ceilCenter, { x: location.SizeX * CM, y: location.SizeY * CM, z: 0.08 }, "08090C", false);

    const wallRun = (alongX, open, fixed) => {
      const length = alongX ? location.SizeX : location.SizeY;
      const place = (alongCenter, alongLength) => {
        if (alongLength < 8) return;
        const center = { X: origin.X, Y: origin.Y, Z: height * 0.5 };
        const scale = { x: 0.2, y: 0.2, z: height * CM };
        if (alongX) {
          center.X += alongCenter;
          center.Y += fixed;
          scale.x = alongLength * CM;
        } else {
          center.Y += alongCenter;
          center.X += fixed;
          scale.y = alongLength * CM;
        }
        this.addSolid(center, scale, "343A44", true);
      };
      if (!open) {
        place(length * 0.5, length);
        return;
      }
      const gap = 180;
      const side = (length - gap) * 0.5;
      place(side * 0.5, side);
      place(length - side * 0.5, side);
    };
    wallRun(true, location.OpenSouth, 0);
    wallRun(true, location.OpenNorth, location.SizeY);
    wallRun(false, location.OpenWest, 0);
    wallRun(false, location.OpenEast, location.SizeX);

    for (const prop of location.Props || []) {
      const scale = { x: prop.Scale.X, y: prop.Scale.Y, z: prop.Scale.Z };
      if (prop.Mesh === "Cylinder") {
        const mesh = makeMesh(new THREE.CylinderGeometry(0.5, 0.5, 1, 12), hexColor(prop.Color));
        mesh.position.copy(ueToThree(prop.Position));
        mesh.scale.set(prop.Scale.X, prop.Scale.Z, prop.Scale.Y);
        this.group.add(mesh);
        if (prop.BlocksPlayer) this.solids.push(boxFromMesh(prop.Position, prop.Scale));
      } else {
        this.addSolid(prop.Position, scale, prop.Color, !!prop.BlocksPlayer);
      }
    }
    for (const light of location.Lights || []) {
      const point = new THREE.PointLight(hexColor(light.Color), Math.max(2.2, light.Intensity / 220), light.Radius * CM, 1.2);
      point.position.copy(ueToThree(light.Position));
      this.scene.add(point);
    }
  }

  buildOfficeWindow(chapter) {
    const windowDef = (chapter.Interactables || []).find((item) => item.Id === "INT_WINDOW");
    if (!windowDef) return;
    const canvas = document.createElement("canvas");
    canvas.width = 256;
    canvas.height = 160;
    this.windowCanvas = canvas;
    const texture = new THREE.CanvasTexture(canvas);
    this.windowTexture = texture;
    const mat = new THREE.MeshBasicMaterial({ map: texture });
    const mesh = new THREE.Mesh(new THREE.PlaneGeometry(1.4, 0.9), mat);
    mesh.position.copy(ueToThree(windowDef.Position));
    mesh.position.z -= 0.05;
    this.group.add(mesh);
    this.paintWindow(0);
  }

  paintWindow(time) {
    if (!this.windowCanvas) return;
    const ctx = this.windowCanvas.getContext("2d");
    ctx.fillStyle = "#070a10";
    ctx.fillRect(0, 0, 256, 160);
    for (let i = 0; i < 18; i += 1) {
      const x = (i * 37) % 240 + 6;
      const h = 30 + ((i * 53) % 90);
      ctx.fillStyle = "#10151c";
      ctx.fillRect(x, 150 - h, 10 + (i % 4) * 3, h);
      if (i % 5 === 0) {
        ctx.fillStyle = i % 2 === 0 ? "#c4a574" : "#3d6d78";
        ctx.fillRect(x + 2, 150 - h + 8, 3, 3);
      }
    }
    ctx.strokeStyle = "rgba(210,220,230,0.35)";
    for (let i = 0; i < 28; i += 1) {
      const x = (i * 19 + time * 40) % 256;
      const y = (i * 47 + time * 90) % 160;
      ctx.beginPath();
      ctx.moveTo(x, y);
      ctx.lineTo(x - 3, y + 14);
      ctx.stroke();
    }
    this.windowTexture.needsUpdate = true;
  }

  refreshInteractables(chapter, defs) {
    this.clearGroup(this.dynamic);
    this.interactables = [];
    this.solids = this.solids.filter((solid) => !solid.dynamic);
    for (const def of defs) {
      let mesh;
      const color = hexColor(def.Color || "22262c");
      const emissiveIntensity = def.Kind === "VoidTrace" ? 0.18 : def.Kind === "StoryObject" ? 0.22 : 0.08;
      if (def.Mesh === "Cylinder") {
        mesh = makeMesh(new THREE.CylinderGeometry(0.5, 0.5, 1, 16), color, { emissiveIntensity, metalness: def.Id === "INT_COIN" ? 0.45 : 0.08 });
        mesh.scale.set(def.Scale.X, def.Scale.Z, def.Scale.Y);
      } else {
        mesh = makeMesh(new THREE.BoxGeometry(1, 1, 1), color, { emissiveIntensity });
        mesh.scale.set(Math.max(def.Scale.X, 0.02), Math.max(def.Scale.Z, 0.02), Math.max(def.Scale.Y, 0.02));
      }
      mesh.position.copy(ueToThree(def.Position));
      this.dynamic.add(mesh);
      const visual = boxFromMesh(def.Position, def.Scale);
      const flat = def.Scale.Z < 0.06 || def.Scale.Y < 0.08;
      const pick = expandBox(visual, def.Hidden ? 0.28 : flat ? 0.62 : 0.42);
      if (def.BlocksPlayer) {
        const solid = { ...visual, dynamic: true };
        this.solids.push(solid);
      }
      this.interactables.push({ def, mesh, pick, visual, phase: 0 });
    }
    this.ensureRain(chapter);
  }

  ensureRain(chapter) {
    if (this.rain) {
      this.scene.remove(this.rain);
      this.rain.geometry.dispose();
      this.rain.material.dispose();
      this.rain = null;
    }
    const count = 700;
    const positions = new Float32Array(count * 3);
    for (let i = 0; i < count; i += 1) {
      positions[i * 3] = 38 + Math.random() * 8;
      positions[i * 3 + 1] = Math.random() * 3.2;
      positions[i * 3 + 2] = 5.6 + Math.random() * 2.8;
    }
    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute("position", new THREE.BufferAttribute(positions, 3));
    const material = new THREE.PointsMaterial({ color: 0xb7c4ce, size: 0.03, transparent: true, opacity: 0.45 });
    this.rain = new THREE.Points(geometry, material);
    this.rain.visible = false;
    this.scene.add(this.rain);
    this.lobbyRain = chapter?.Locations?.some((loc) => loc.LocationId === "LOC_LOBBY");
  }

  setRainVisible(on) {
    if (this.rain) this.rain.visible = !!on;
  }

  spawnPose(location) {
    this.position.copy(ueToThree(location.Spawn));
    this.position.y = Math.max(this.position.y, 0.88);
    this.yaw = ((location.SpawnYaw || 0) * Math.PI) / 180;
    this.pitch = -0.08;
    this.velocity.set(0, 0, 0);
    this.player.position.copy(this.position);
  }

  teleportUe(vec, yawDeg) {
    this.position.copy(ueToThree(vec));
    this.position.y = Math.max(0.88, this.position.y);
    if (typeof yawDeg === "number") this.yaw = (yawDeg * Math.PI) / 180;
    this.velocity.set(0, 0, 0);
    this.player.position.copy(this.position);
  }

  poseUe() {
    return {
      x: this.position.x / CM,
      y: this.position.z / CM,
      z: this.position.y / CM,
      yaw: (this.yaw * 180) / Math.PI,
      pitch: (this.pitch * 180) / Math.PI,
    };
  }

  applyPose(pose) {
    if (!pose) return;
    this.position.set(pose.x * CM, Math.max(0.88, (pose.z || 88) * CM), pose.y * CM);
    this.yaw = ((pose.yaw || 0) * Math.PI) / 180;
    this.pitch = ((pose.pitch || 0) * Math.PI) / 180;
    this.player.position.copy(this.position);
  }

  setCameraMode(mode) {
    if (mode === "Close") {
      this.targetArm = 1.6;
      this.socket.set(0, 0.28, 0.58);
    } else if (mode === "Low") {
      this.targetArm = 2.4;
      this.socket.set(0, 0.36, 0.18);
    } else if (mode === "Conversation") {
      this.targetArm = 1.9;
      this.socket.set(0, 0.62, 0.52);
    } else if (mode === "Corridor") {
      this.targetArm = 2.0;
      this.socket.set(0, 0.22, 0.58);
    } else if (mode === "Flashback") {
      this.targetArm = 2.2;
      this.socket.set(0, 0.36, 0.5);
    } else {
      this.targetArm = 2.8;
      this.socket.set(0, 0.48, 0.62);
    }
  }

  setSaturation(value) {
    this.saturation = value;
    this.canvas.style.filter = `saturate(${value}) contrast(1.14)`;
  }

  collide() {
    const radius = 0.32;
    for (let iter = 0; iter < 3; iter += 1) {
      for (const box of this.solids) {
        if (box.max.y < 0.25 || box.min.y > 1.75) continue;
        const cx = Math.max(box.min.x, Math.min(this.position.x, box.max.x));
        const cz = Math.max(box.min.z, Math.min(this.position.z, box.max.z));
        let dx = this.position.x - cx;
        let dz = this.position.z - cz;
        const d2 = dx * dx + dz * dz;
        if (d2 >= radius * radius) continue;
        if (d2 < 1e-8) {
          const left = this.position.x - box.min.x;
          const right = box.max.x - this.position.x;
          const back = this.position.z - box.min.z;
          const fwd = box.max.z - this.position.z;
          const m = Math.min(left, right, back, fwd);
          if (m === left) this.position.x = box.min.x - radius;
          else if (m === right) this.position.x = box.max.x + radius;
          else if (m === back) this.position.z = box.min.z - radius;
          else this.position.z = box.max.z + radius;
        } else {
          const d = Math.sqrt(d2);
          const push = radius - d + 0.001;
          this.position.x += (dx / d) * push;
          this.position.z += (dz / d) * push;
        }
      }
    }
  }

  update(dt, input, blocked) {
    const forward = new THREE.Vector3(Math.cos(this.yaw), 0, Math.sin(this.yaw));
    const right = new THREE.Vector3(-Math.sin(this.yaw), 0, Math.cos(this.yaw));
    if (!blocked && input) {
      this.yaw -= input.lookX || 0;
      this.pitch = Math.max(-0.85, Math.min(0.55, this.pitch + (input.lookY || 0)));
      const wish = new THREE.Vector3();
      wish.addScaledVector(forward, input.forward || 0);
      wish.addScaledVector(right, input.strafe || 0);
      if (wish.lengthSq() > 1) wish.normalize();
      const speed = input.sprint ? 5.2 : 1.9;
      const desired = wish.multiplyScalar(speed);
      const rate = wish.lengthSq() > 0 ? 3.1 : 4.8;
      this.velocity.lerp(desired, 1 - Math.exp(-rate * dt));
    } else {
      this.velocity.lerp(new THREE.Vector3(), 1 - Math.exp(-6 * dt));
    }
    const steps = Math.max(1, Math.ceil(this.velocity.length() * dt / 0.05));
    const step = this.velocity.clone().multiplyScalar(dt / steps);
    for (let i = 0; i < steps; i += 1) {
      this.position.add(step);
      this.position.y = 0.88;
      this.collide();
    }
    this.player.position.copy(this.position);
    this.player.rotation.y = -this.yaw;

    const corridor = this.locationId === "LOC_CORRIDOR" || this.locationId === "LOC_STAIR";
    const desiredArm = corridor ? Math.min(this.targetArm, 2.1) : this.targetArm;
    this.arm += (desiredArm - this.arm) * Math.min(1, dt * 4);

    const look = new THREE.Vector3(Math.cos(this.yaw) * Math.cos(this.pitch), Math.sin(this.pitch), Math.sin(this.yaw) * Math.cos(this.pitch));
    const pivot = this.position.clone().add(new THREE.Vector3(0, this.socket.z, 0)).add(right.multiplyScalar(this.socket.y));
    let camPos = pivot.clone().addScaledVector(look, -this.arm);
    const camDir = camPos.clone().sub(pivot);
    const dist = camDir.length();
    if (dist > 0.001) {
      camDir.multiplyScalar(1 / dist);
      let hit = dist;
      for (const box of this.solids) {
        const t = rayBox(pivot, camDir, box, dist);
        if (t !== null && t > 0.05 && t < hit) hit = t;
      }
      camPos = pivot.clone().addScaledVector(camDir, Math.max(0.35, hit - 0.12));
    }
    this.camera.position.copy(camPos);
    this.camera.up.set(0, 1, 0);
    this.camera.lookAt(pivot.clone().add(look));

    this.focus = null;
    if (!blocked) {
      const look = new THREE.Vector3(Math.cos(this.yaw) * Math.cos(this.pitch), Math.sin(this.pitch), Math.sin(this.yaw) * Math.cos(this.pitch));
      const origin = this.position.clone();
      origin.y = 1.35;
      origin.addScaledVector(look, 0.2);
      let best = 3.6;
      let found = null;
      for (const item of this.interactables) {
        const t = rayBox(origin, look, item.pick, best);
        if (t === null) continue;
        if (item.def.Hidden) {
          const dx = item.mesh.position.x - this.position.x;
          const dz = item.mesh.position.z - this.position.z;
          if (dx * dx + dz * dz > 1.3 * 1.3) continue;
        }
        best = t;
        found = item;
      }
      this.focus = found;
    }

    const time = this.clock.getElapsedTime();
    for (const item of this.interactables) {
      if (item.def.Kind === "VoidTrace") {
        item.phase += dt;
        const flicker = 1 + 0.04 * Math.sin(item.phase * 9);
        const s = item.def.Scale;
        item.mesh.scale.set(Math.max(s.X, 0.02) * flicker, Math.max(s.Z, 0.02) * flicker, Math.max(s.Y, 0.02) * flicker);
      }
      const focused = this.focus === item;
      item.mesh.material.emissiveIntensity = focused ? 0.45 : item.def.Kind === "VoidTrace" ? 0.16 : 0.08;
    }
    if (this.rain?.visible) {
      const attr = this.rain.geometry.attributes.position;
      for (let i = 0; i < attr.count; i += 1) {
        let y = attr.getY(i) - dt * 3.4;
        if (y < 0.05) y = 3.2;
        attr.setY(i, y);
      }
      attr.needsUpdate = true;
    }
    this.paintWindow(time);
    this.renderer.render(this.scene, this.camera);
  }

  project(vec3) {
    const width = this.canvas.clientWidth || window.innerWidth;
    const height = this.canvas.clientHeight || window.innerHeight;
    const projected = vec3.clone().project(this.camera);
    return {
      x: (projected.x * 0.5 + 0.5) * width,
      y: (-projected.y * 0.5 + 0.5) * height,
      behind: projected.z > 1,
    };
  }
}
