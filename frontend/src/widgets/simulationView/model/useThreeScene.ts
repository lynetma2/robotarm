import { onMounted, onUnmounted, type Ref } from 'vue'
import * as THREE from 'three'
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js'

export function useThreeScene(containerRef: Ref<HTMLElement | null>) {
  let scene: THREE.Scene
  let camera: THREE.PerspectiveCamera
  let renderer: THREE.WebGLRenderer
  let controls: OrbitControls
  let animationId: number
  let resizeObserver: ResizeObserver

  const init = () => {
    if (!containerRef.value) return

    // 1. Scene & Fog
    scene = new THREE.Scene()
    scene.background = new THREE.Color('#f5f5f5')
    scene.fog = new THREE.Fog('#f5f5f5', 20, 100)

    // 2. Camera
    const width = containerRef.value.clientWidth
    const height = containerRef.value.clientHeight
    camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 1000)
    camera.position.set(20, 20, 30)
    camera.lookAt(0, 0, 0)

    // 3. Renderer
    renderer = new THREE.WebGLRenderer({ antialias: true })
    renderer.setSize(width, height)
    renderer.shadowMap.enabled = true
    containerRef.value.appendChild(renderer.domElement)

    // 4. Controls
    controls = new OrbitControls(camera, renderer.domElement)
    controls.enableDamping = true
    controls.dampingFactor = 0.05

    // 5. Environment (Lights, Grid, Base)
    setupEnvironment(scene)

    // 6. Animation Loop
    const animate = () => {
      animationId = requestAnimationFrame(animate)
      controls.update()
      renderer.render(scene, camera)
    }
    animate()

    // 7. Resize Handler
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

  // Helper to keep init() clean
  const setupEnvironment = (scene: THREE.Scene) => {
    // Grid & Axes
    scene.add(new THREE.GridHelper(50, 50, 0x888888, 0xe5e5e5))
    scene.add(new THREE.AxesHelper(5))

    // Lights
    scene.add(new THREE.AmbientLight(0xffffff, 0.6))
    const dirLight = new THREE.DirectionalLight(0xffffff, 0.8)
    dirLight.position.set(10, 20, 10)
    dirLight.castShadow = true
    scene.add(dirLight)

    // Robot Base Placeholder
    const geometry = new THREE.CylinderGeometry(2, 2.5, 1, 32)
    const material = new THREE.MeshStandardMaterial({ color: 0x3b82f6 })
    const baseMesh = new THREE.Mesh(geometry, material)
    baseMesh.position.y = 0.5
    baseMesh.castShadow = true
    scene.add(baseMesh)
  }

  // Lifecycle
  onMounted(init)

  onUnmounted(() => {
    cancelAnimationFrame(animationId)
    resizeObserver?.disconnect()
    renderer?.dispose()
    controls?.dispose()
    if (containerRef.value && renderer?.domElement) {
      containerRef.value.removeChild(renderer.domElement)
    }
  })
}
