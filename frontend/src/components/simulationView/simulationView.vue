<script setup lang="ts">
import { ref, onMounted, onUnmounted, watch } from 'vue'
import * as THREE from 'three'
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'

// --- Props (for future connection to your real data) ---
const props = defineProps<{
  jointAngles?: number[] // Example: [0, 90, -45, 0, 0, 0]
}>()

// --- Mock Data for Overlay ---
const coordinates = ref([
  { label: 'X', value: '120.5' },
  { label: 'Y', value: '-45.2' },
  { label: 'Z', value: '300.0' },
  { label: 'R', value: '0.00' },
  { label: 'A', value: '90.0' },
  { label: 'B', value: '12.5' },
])

// --- Three.js Refs ---
const containerRef = ref<HTMLDivElement | null>(null)
let scene: THREE.Scene
let camera: THREE.PerspectiveCamera
let renderer: THREE.WebGLRenderer
let controls: OrbitControls
let animationId: number
let resizeObserver: ResizeObserver

// --- Initialization ---
const initThree = () => {
  if (!containerRef.value) return

  // 1. Scene
  scene = new THREE.Scene()
  scene.background = new THREE.Color('#f5f5f5') // Light gray background
  // Add some fog for depth
  scene.fog = new THREE.Fog('#f5f5f5', 20, 100)

  // 2. Camera
  const width = containerRef.value.clientWidth
  const height = containerRef.value.clientHeight
  camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 1000)
  camera.position.set(20, 20, 30) // Offset view
  camera.lookAt(0, 0, 0)

  // 3. Renderer
  renderer = new THREE.WebGLRenderer({ antialias: true })
  renderer.setSize(width, height)
  renderer.shadowMap.enabled = true
  containerRef.value.appendChild(renderer.domElement)

  // 4. Controls (Orbit)
  controls = new OrbitControls(camera, renderer.domElement)
  controls.enableDamping = true // Smooth motion
  controls.dampingFactor = 0.05

  // 5. Helpers (Grid & Axes)
  // Grid: size 50, divisions 50
  const gridHelper = new THREE.GridHelper(50, 50, 0x888888, 0xe5e5e5)
  scene.add(gridHelper)

  // Axes: Red=X, Green=Y, Blue=Z
  const axesHelper = new THREE.AxesHelper(5)
  scene.add(axesHelper)

  // 6. Lighting
  const ambientLight = new THREE.AmbientLight(0xffffff, 0.6)
  scene.add(ambientLight)

  const dirLight = new THREE.DirectionalLight(0xffffff, 0.8)
  dirLight.position.set(10, 20, 10)
  dirLight.castShadow = true
  scene.add(dirLight)

  // 7. Placeholder Object (Representing Robot Base)
  const geometry = new THREE.CylinderGeometry(2, 2.5, 1, 32)
  const material = new THREE.MeshStandardMaterial({ color: 0x3b82f6 }) // Blue
  const baseMesh = new THREE.Mesh(geometry, material)
  baseMesh.position.y = 0.5
  baseMesh.castShadow = true
  scene.add(baseMesh)

  // 8. Animation Loop
  const animate = () => {
    animationId = requestAnimationFrame(animate)
    controls.update()
    renderer.render(scene, camera)
  }
  animate()

  // 9. Handle Resize using ResizeObserver
  // This is better than window.resize because the grid container might change
  resizeObserver = new ResizeObserver(() => {
    if (!containerRef.value) return
    const newWidth = containerRef.value.clientWidth
    const newHeight = containerRef.value.clientHeight

    camera.aspect = newWidth / newHeight
    camera.updateProjectionMatrix()
    renderer.setSize(newWidth, newHeight)
  })
  resizeObserver.observe(containerRef.value)
}

// --- Lifecycle Hooks ---
onMounted(() => {
  initThree()
})

onUnmounted(() => {
  cancelAnimationFrame(animationId)
  resizeObserver?.disconnect()
  renderer?.dispose()
  controls?.dispose()
  // Clean up DOM
  if (containerRef.value && renderer?.domElement) {
    containerRef.value.removeChild(renderer.domElement)
  }
})
</script>

<template>
  <Card class="flex flex-col h-full bg-card text-card-foreground shadow-sm overflow-hidden">
    <CardHeader class="pb-2 border-b">
      <CardTitle class="text-xl font-semibold flex justify-between items-center">
        <span>3D View & Simulation</span>
        <span class="text-xs font-normal text-muted-foreground bg-secondary px-2 py-1 rounded">
          Interactive
        </span>
      </CardTitle>
    </CardHeader>

    <CardContent class="flex-grow relative p-0 overflow-hidden">

      <div ref="containerRef" class="w-full h-full cursor-move outline-none" />

      <div class="absolute bottom-6 left-1/2 -translate-x-1/2 bg-white/90 dark:bg-zinc-950/90 backdrop-blur border rounded-lg p-4 shadow-xl w-[90%] max-w-3xl pointer-events-none select-none">
        <h4 class="text-xs font-bold mb-3 text-muted-foreground uppercase tracking-wider flex items-center gap-2">
          <span class="w-2 h-2 rounded-full bg-green-500 animate-pulse"></span>
          Current coordinates
        </h4>
        <div class="grid grid-cols-6 gap-2 pointer-events-auto">
          <div
            v-for="(axis, index) in coordinates"
            :key="index"
            class="flex flex-col items-center justify-center p-2 bg-background/50 border rounded-md hover:bg-background hover:border-primary/50 transition-all shadow-sm"
          >
            <span class="text-[10px] uppercase text-muted-foreground font-semibold">{{ axis.label }}</span>
            <span class="font-mono font-medium text-lg tracking-tight">{{ axis.value }}</span>
          </div>
        </div>
      </div>

    </CardContent>
  </Card>
</template>
