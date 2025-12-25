import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { Sequence } from '@/entities/sequence/types'

// Define the shape of our pose (matching your existing types)
interface Pose {
  x: number; y: number; z: number;
  roll: number; pitch: number; yaw: number;
}

export const useSimulationStore = defineStore('simulation', () => {
  const isSimulating = ref(false)
  const currentPose = ref<Pose>({ x: 0, y: 0, z: 0, roll: 0, pitch: 0, yaw: 0 })
  const progress = ref(0)

  let animationFrameId: number
  let startTime: number
  let trajectory: Pose[] = []
  let duration = 5000 // ms

  // --- Actions ---

  const startSimulation = (sequence: Sequence) => {
    // 1. In a real app, you would fetch the calculated trajectory from the backend here.
    //    const response = await api.post('/simulate', sequence)
    //    trajectory = response.data

    // 2. For now, we generate a simple linear interpolation between steps for visualization
    trajectory = generateMockTrajectory(sequence)

    isSimulating.value = true
    progress.value = 0
    startTime = performance.now()
    duration = sequence.steps.length * 1000 // 1 second per step approx

    loop()
  }

  const stopSimulation = () => {
    isSimulating.value = false
    cancelAnimationFrame(animationFrameId)
  }

  const loop = () => {
    const now = performance.now()
    const elapsed = now - startTime
    const t = Math.min(elapsed / duration, 1) // Normalized time 0..1

    progress.value = t * 100

    // Sample the trajectory
    if (trajectory.length > 0) {
      const index = Math.floor(t * (trajectory.length - 1))
      currentPose.value = trajectory[index]
    }

    if (t < 1) {
      animationFrameId = requestAnimationFrame(loop)
    } else {
      stopSimulation()
    }
  }

  // --- Helper: Mock Trajectory Generator ---
  // Generates 60 frames per step to simulate smooth movement
  const generateMockTrajectory = (seq: Sequence): Pose[] => {
    const frames: Pose[] = []
    const stepsPerSegment = 60

    for (let i = 0; i < seq.steps.length - 1; i++) {
      const start = seq.steps[i].pose
      const end = seq.steps[i + 1].pose

      for (let j = 0; j <= stepsPerSegment; j++) {
        const t = j / stepsPerSegment
        frames.push({
          x: lerp(start.x, end.x, t),
          y: lerp(start.y, end.y, t),
          z: lerp(start.z, end.z, t),
          roll: lerp(start.roll, end.roll, t),
          pitch: lerp(start.pitch, end.pitch, t),
          yaw: lerp(start.yaw, end.yaw, t),
        })
      }
    }
    return frames
  }

  const lerp = (start: number, end: number, t: number) => {
    return start * (1 - t) + end * t
  }

  return {
    isSimulating,
    currentPose,
    progress,
    startSimulation,
    stopSimulation
  }
})
