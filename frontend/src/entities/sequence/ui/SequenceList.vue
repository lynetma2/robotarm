<script setup lang="ts">
import { Play, Pencil } from 'lucide-vue-next'
import { Button } from '@/shared/ui/button'
import type { Sequence } from '../model/types'

// Props: Data only
defineProps<{
  sequences: Sequence[]
}>()

// Emits: Actions only
const emit = defineEmits<{
  (e: 'edit', sequence: Sequence): void
  (e: 'play', id: number): void
}>()
</script>

<template>
  <div class="grid gap-4">
    <div
      v-for="item in sequences"
      :key="item.id"
      class="flex items-center justify-between p-4 border rounded-lg hover:bg-accent/50 transition-colors"
    >
      <div class="grid gap-1">
        <h3 class="font-medium leading-none">{{ item.title }}</h3>
        <p class="text-sm text-muted-foreground">{{ item.description }}</p>
      </div>

      <div class="flex items-center gap-2">
        <Button size="icon" variant="ghost" @click="emit('play', item.id!)">
          <Play class="w-4 h-4" />
        </Button>
        <Button size="icon" variant="outline" @click="emit('edit', item)">
          <Pencil class="w-4 h-4" />
        </Button>
      </div>
    </div>
  </div>
</template>
