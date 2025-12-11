<script setup lang="ts">
import draggable from 'vuedraggable'
import { GripVertical, Trash2, Plus, Save } from 'lucide-vue-next'

// Shared UI
import { Button } from '@/shared/ui/button'
import { Input } from '@/shared/ui/input'
import { Label } from '@/shared/ui/label'
import { Accordion, AccordionContent, AccordionItem, AccordionTrigger } from '@/shared/ui/accordion'

// Entity Types
import type { Sequence, Step } from '@/entities/sequence/model/types'

const props = defineProps<{
  sequence: Sequence
}>()

const emit = defineEmits<{
  (e: 'delete-sequence', id: number): void
  (e: 'update-sequence', sequence: Sequence): void
}>()

// --- Local Actions ---

const addWaypoint = () => {
  const newStep: Step = {
    name: "New Step",
    pose: { x: 0, y: 0, z: 0, roll: 0, pitch: 0, yaw: 0 },
    speed: 100,
    interpolation: 'LINEAR'
  }
  props.sequence.steps.push(newStep)
}

const removeWaypoint = (index: number) => {
  props.sequence.steps.splice(index, 1)
}

const saveList = () => {
  emit('update-sequence', props.sequence)
}
</script>

<template>
  <div class="space-y-6">

    <div class="grid gap-3 p-4 border rounded-lg bg-muted/20">
      <div class="grid gap-1">
        <Label>Sequence Title</Label>
        <Input v-model="props.sequence.title" class="bg-background" />
      </div>
      <div class="grid gap-1">
        <Label>Description</Label>
        <Input v-model="props.sequence.description" class="bg-background" />
      </div>
    </div>

    <draggable
      v-model="props.sequence.steps"
      item-key="id"
      handle=".drag-handle"
      animation="200"
      class="space-y-2"
    >
      <template #item="{ element: point, index }">
        <div class="border rounded-md bg-card">
          <Accordion type="single" collapsible class="w-full">
            <AccordionItem :value="String(index)" class="border-0">

              <div class="flex items-center px-4 py-2">
                <div class="drag-handle cursor-grab active:cursor-grabbing mr-4 text-muted-foreground hover:text-foreground">
                  <GripVertical class="w-5 h-5" />
                </div>

                <AccordionTrigger class="flex-1 py-0 hover:no-underline">
                  <span class="text-sm font-medium">
                    Step {{ index + 1 }}
                    <span v-if="point.id" class="text-muted-foreground font-normal ml-2">(ID: {{ point.id }})</span>
                  </span>
                  <span class="ml-auto mr-4 text-xs text-muted-foreground">
                    X: {{ point.pose.x }} / Y: {{ point.pose.y }}
                  </span>
                </AccordionTrigger>
              </div>

              <AccordionContent class="px-4 pb-4 border-t bg-muted/20">
                <div class="grid grid-cols-3 gap-4 pt-4">
                  <div class="grid gap-2">
                    <Label>Position X</Label>
                    <Input type="number" v-model="point.pose.x" />
                  </div>
                  <div class="grid gap-2">
                    <Label>Position Y</Label>
                    <Input type="number" v-model="point.pose.y" />
                  </div>
                  <div class="grid gap-2">
                    <Label>Speed</Label>
                    <Input type="number" v-model="point.speed" />
                  </div>
                </div>

                <div class="flex justify-end mt-4">
                  <Button
                    variant="destructive"
                    size="sm"
                    class="text-white"
                    @click="removeWaypoint(index)"
                  >
                    <Trash2 class="w-4 h-4 mr-2"/> Remove Step
                  </Button>
                </div>
              </AccordionContent>

            </AccordionItem>
          </Accordion>
        </div>
      </template>
    </draggable>

    <Button class="w-full" variant="outline" border="dashed" @click="addWaypoint">
      <Plus class="w-4 h-4 mr-2" /> Add Waypoint Step
    </Button>

    <div class="border-t pt-6 mt-6 flex flex-col gap-3">
      <Button class="w-full" @click="saveList">
        <Save class="w-4 h-4 mr-2" /> Save Changes
      </Button>

      <Button
        v-if="props.sequence.id"
        variant="destructive"
        class="w-full text-white"
        @click="$emit('delete-sequence', props.sequence.id)"
      >
        <Trash2 class="w-4 h-4 mr-2" /> Delete Entire Sequence
      </Button>
    </div>

  </div>
</template>
