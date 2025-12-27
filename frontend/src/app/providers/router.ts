import { createRouter, createWebHistory, type RouteRecordRaw } from 'vue-router'

const routes: RouteRecordRaw[] = [
  {
    path: '/',
    name: 'Dashboard',
    component: () => import('@/pages/dashboard/DashboardPage.vue'),
  },
  {
    path: '/editor',
    name: 'Editor',
    component: () => import('@/pages/editor/EditorPage.vue'),
  },
  {
    path: '/motorConfig',
    name: 'MotorConfig',
    component: () => import('@/pages/motorConfigPage.vue'),
  },
]

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes,
})

export default router
