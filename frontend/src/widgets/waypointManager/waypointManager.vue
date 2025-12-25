<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { ArrowLeft, Plus, PlayCircle } from 'lucide-vue-next'
// Shared UI
import { Button } from '@/shared/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/shared/ui/card'
// Shared API
import { useStomp } from '@/shared/api/useStomp'
import { useSimulationStore } from '@/entities/simulation/model/store'

// FSD Layers
import SequenceList from '@/entities/sequence/ui/SequenceList.vue'
import SequenceEditor from '@/features/manage-sequence/ui/SequenceEditor.vue'
import type { Sequence } from '@/entities/sequence/types'

// --- State ---
const selectedSequence = ref<Sequence | null>(null)
const sequences = ref<Sequence[]>([])
const simulationStore = useSimulationStore()

// --- API Integration ---
const { publish } = useStomp() // Keep for Play command
const API_BASE = '/api/v1/sequences'

const fetchSequences = async () => {
  try {
    const res = await fetch(API_BASE)
    if (res.ok) {
      sequences.value = await res.json()
    }
  } catch (e) {
    console.error("Failed to fetch sequences", e)
  }
}

onMounted(() => {
  fetchSequences()
})

// --- Handlers ---
// (Keep the rest of your handlers exactly as they were)
const handleEdit = (seq: Sequence) => {
  selectedSequence.value = JSON.parse(JSON.stringify(seq))
}

const handleAdd = async () => {
  const newSequencePayload: Sequence = {
    title: 'New Sequence',
    description: 'Enter description here...',
    steps: [{
      name: "New Step",
      pose: { x: 0, y: 0, z: 0, roll: 0, pitch: 0, yaw: 0 },
      interpolation: 'LINEAR',
      speed: 100
    }],
    settings: { loop: false }
  }
  
  try {
    const res = await fetch(API_BASE, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(newSequencePayload)
    })
    if (res.ok) sequences.value = await res.json()
  } catch (e) {
    console.error("Failed to create sequence", e)
  }
}

const handleUpdateSequence = async (updatedSequence: Sequence) => {
  try {
    const res = await fetch(`${API_BASE}/${updatedSequence.id}`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(updatedSequence)
    })
    if (res.ok) {
      sequences.value = await res.json()
      selectedSequence.value = null
    }
  } catch (e) {
    console.error("Failed to update sequence", e)
  }
}

const handleDeleteSequence = async (id: number) => {
  try {
    const res = await fetch(`${API_BASE}/${id}`, { method: 'DELETE' })
    if (res.ok) {
      sequences.value = await res.json()
      selectedSequence.value = null
    }
  } catch (e) {
    console.error("Failed to delete sequence", e)
  }
}

const handlePlaySequence = (id: number) => {
  publish(`/app/sequences/${id}/play`, {})
}

const handleSimulate = () => {
  if (selectedSequence.value) simulationStore.startSimulation(selectedSequence.value)
}
</script>

<template>
  <Card class="flex flex-col bg-card h-full">
    <CardHeader class="flex flex-row items-center space-y-0 gap-4 border-b pb-4">
      <Button
        v-if="selectedSequence"
        variant="outline"
        size="icon"
        @click="selectedSequence = null"
      >
        <ArrowLeft class="w-4 h-4" />
      </Button>

      <div class="flex flex-col">
        <div class="flex items-center gap-2">
          <CardTitle>
            {{ selectedSequence ? `Editing: ${selectedSequence.title}` : 'Waypoint Manager' }}
          </CardTitle>
          <!-- Simulation Button -->
          <Button v-if="selectedSequence" variant="ghost" size="sm" class="h-6 text-blue-600 hover:text-blue-700 hover:bg-blue-50" @click="handleSimulate">
            <PlayCircle class="w-3.5 h-3.5 mr-1" /> Simulate
          </Button>
        </div>
        <CardDescription>
          {{ selectedSequence ? 'Drag to reorder. Expand to edit.' : 'Save and edit movements.' }}
        </CardDescription>
      </div>

      <div v-if="!selectedSequence" class="ml-auto">
        <Button variant="outline" size="icon" @click="handleAdd">
          <Plus class="w-4 h-4" />
        </Button>
      </div>
    </CardHeader>

    <CardContent class="p-0 overflow-hidden flex-1">
      <div class="h-full overflow-y-auto p-4">

        <SequenceEditor
          v-if="selectedSequence"
          :sequence="selectedSequence"
          @delete-sequence="handleDeleteSequence"
          @update-sequence="handleUpdateSequence"
        />

        <SequenceList
          v-else
          :sequences="sequences"
          @edit="handleEdit"
          @play="handlePlaySequence"
        />

      </div>
    </CardContent>
  </Card>
</template>
