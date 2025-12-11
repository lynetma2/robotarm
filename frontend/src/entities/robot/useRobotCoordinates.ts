import { ref } from 'vue'
import { useStomp } from '@/shared/api/useStomp'
import type { Coordinate } from './types'

export function useRobotCoordinates() {
  const { subscribe } = useStomp()

  // Default State
  const coordinates = ref<Coordinate[]>([
    { label: 'X', value: '0.00' },
    { label: 'Y', value: '0.00' },
    { label: 'Z', value: '0.00' },
    { label: 'R', value: '0.00' },
    { label: 'P', value: '0.00' },
    { label: 'Y', value: '0.00' },
  ])

  // Subscribe to live updates
  subscribe('/topic/coordinates', (payload: Coordinate[]) => {
    coordinates.value = payload
  })

  return { coordinates }
}
