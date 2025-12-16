import * as THREE from 'three'
import { onMounted, onUnmounted, type Ref } from 'vue'
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls'

export function useThreeScene(containerRef: Ref<HTMLElement | null>) {
  let scene: THREE.Scene
  let camera: THREE.PerspectiveCamera
  let renderer: THREE.WebGLRenderer
  let controls: OrbitControls
  let animationId: number
  let resizeObserver: ResizeObserver

  const init = () => {
    if (!containerRef.value) return

    // 1. Setup Scene
    scene = new THREE.Scene()
    // Use a transparent background so it blends with the card color, 
    // or set a specific color like new THREE.Color('#18181b'). 
    // We leave it null for transparency to let CSS bg-card show through.

    // 2. Setup Camera
    const width = containerRef.value.clientWidth
    const height = containerRef.value.clientHeight
    camera = new THREE.PerspectiveCamera(45, width / height, 0.1, 1000)
    camera.position.set(30, 30, 40)
    camera.lookAt(0, 0, 0)

    // 3. Setup Renderer
    renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true })
    renderer.setSize(width, height)
    renderer.setPixelRatio(window.devicePixelRatio)
    renderer.shadowMap.enabled = true
    containerRef.value.appendChild(renderer.domElement)

    // 4. Controls
    controls = new OrbitControls(camera, renderer.domElement)
    controls.enableDamping = true
    controls.dampingFactor = 0.05

    // 5. Lighting
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.4)
    scene.add(ambientLight)

    const dirLight = new THREE.DirectionalLight(0xffffff, 1)
    dirLight.position.set(10, 20, 10)
    dirLight.castShadow = true
    scene.add(dirLight)

    // 6. Helpers (Grid)
    const gridHelper = new THREE.GridHelper(100, 100, 0x444444, 0x222222)
    scene.add(gridHelper)

    const axesHelper = new THREE.AxesHelper(5)
    scene.add(axesHelper)

    animate()
  }

  const animate = () => {
    animationId = requestAnimationFrame(animate)
    controls.update()
    renderer.render(scene, camera)
  }

  const handleResize = () => {
    if (!containerRef.value || !camera || !renderer) return
    const width = containerRef.value.clientWidth
    const height = containerRef.value.clientHeight
    
    camera.aspect = width / height
    camera.updateProjectionMatrix()
    renderer.setSize(width, height)
  }

  onMounted(() => {
    init()
    
    // Robust resizing using ResizeObserver
    resizeObserver = new ResizeObserver(() => handleResize())
    if (containerRef.value) {
      resizeObserver.observe(containerRef.value)
    }
  })

  onUnmounted(() => {
    cancelAnimationFrame(animationId)
    resizeObserver?.disconnect()
    renderer?.dispose()
    controls?.dispose()
  })

  return {
    // Return these so you can add objects to the scene later
    getScene: () => scene,
    getCamera: () => camera
  }
}