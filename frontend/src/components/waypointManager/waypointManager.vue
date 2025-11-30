<script setup lang="ts">
import { ref } from 'vue'
import { ArrowLeft, Plus } from 'lucide-vue-next'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'

// Import Sub-components and Types
import WaypointList from './waypointList.vue'
import WaypointEditor from './waypointEditor.vue'
import { useStomp } from '@/composable/useStomp'
import type {Sequence} from "@/types/robotarm.ts";

// --- State ---
const selectedSequence = ref<Sequence | null>(null)
const sequences = ref<Sequence[]>([])

// --- WebSocket Integration ---
const { publish, subscribe } = useStomp()

// 1. Subscription
// Now safe to call at the top level.
// - It automatically waits for the connection.
// - It automatically registers cleanup when this component unmounts.
subscribe('/topic/sequences', (data: Sequence[]) => {
  sequences.value = data
})

// 2. Initial Data Fetch
// Now safe to call immediately.
// - If connected, it sends.
// - If disconnected, it waits and sends as soon as the connection opens.
publish('/app/sequences/get', {})

// --- Handlers ---

// Enter Edit Mode
const handleEdit = (seq: Sequence) => {
  selectedSequence.value = seq
}

// Exit Edit Mode
const handleBack = () => {
  selectedSequence.value = null
}

// Create New Sequence
const handleAdd = () => {
  const newSequencePayload = {
    title: 'New Sequence',
    description: 'Enter description here...',
    steps: [{
      name: "New Step",
      pose: {
        x: 0,
        y: 0,
        z: 0,
        roll: 0,
        pitch: 0,
        yaw: 0
      },
      interpolation: 'LINEAR',
      speed: 100
    }],
    settings: {
      loop: false
    },
  }
  // Publish an event to the backend to create a new sequence.
  publish('/app/sequences/create', newSequencePayload)
}

// Delete Sequence (Triggered by Editor)
const handleDeleteSequence = (id: number) => {
  publish('/app/sequences/delete', { id })
  // Close the editor
  selectedSequence.value = null
}

// Update Sequence (Triggered by Editor)
const handleUpdateSequence = (updatedSequence: Sequence) => {
  // Publish the entire updated sequence object to the backend.
  publish('/app/sequences/' + updatedSequence.id + '/update', updatedSequence)
  // Exit the editor and return to the list view as user feedback
  selectedSequence.value = null
}

// Play Sequence
const handlePlaySequence = (id: number) => {
  console.log('Playing sequence via WebSocket:', id)
  publish('/app/sequences/play', { id })
}
</script>

<template>
  <Card class="flex flex-col bg-card">

    <CardHeader class="flex flex-row items-center space-y-0 gap-4">
      <Button
        v-if="selectedSequence"
        variant="outline"
        size="icon"
        @click="handleBack"
      >
        <ArrowLeft class="w-4 h-4" />
      </Button>

      <div class="flex flex-col">
        <CardTitle>
          {{ selectedSequence ? `Editing: ${selectedSequence.title}` : 'Waypoint Manager' }}
        </CardTitle>
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

    <CardContent>
      <div class="max-h-[400px] overflow-y-auto pr-4">

        <WaypointEditor
          v-if="selectedSequence"
          :sequence="selectedSequence"
          @delete-sequence="handleDeleteSequence"
          @update-sequence="handleUpdateSequence"
        />

        <WaypointList
          v-else
          :sequences="sequences"
          @edit="handleEdit"
          @play="handlePlaySequence"
        />

      </div>
    </CardContent>

  </Card>
</template>
